#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * The elimination-backoff stack of The Art of Multiprocessor Programming,
 * ch. 11 (Figs. 11.5-11.8): a Treiber stack, plus an exchanger where a push
 * and a pop that failed their CAS on top can meet and cancel out. The
 * exchanger slot holds an item and a state (EMPTY, WAITING, BUSY) in one
 * word, like Java's AtomicStampedReference. A thread finding the slot EMPTY
 * installs its item as WAITING and waits; one finding it WAITING swaps in
 * its own item as BUSY and takes the waiting one; the waiter then sees BUSY,
 * takes that item and empties the slot. A waiter that gives up (its timeout
 * expires, here at any check) cancels with a CAS from WAITING back to EMPTY;
 * if the CAS fails, a partner has just arrived and the exchange completes.
 * A push exchanging with a pop is eliminated; any other outcome retries.
 *
 * The CASes on top are acq_rel, so that pushes and pops synchronize with
 * each other as the relinche specs with synchronization require. Values
 * are nonzero; 0 is the empty item, and pop() returns 0 on an empty stack.
 * Nodes are not reclaimed. Also included by the relinche stack client.
 *
 * BUG_TIMEOUT_STORE: the giving-up waiter empties the slot with a plain
 * store instead of the CAS (TAMP explains why it must be a CAS). If a pop
 * arrived meanwhile and took the waiting push's value, the pusher still
 * believes it timed out and pushes the value again: it is popped twice.
 */

#define EMPTY 0
#define WAITING 1
#define BUSY 2

struct node {
	unsigned int value;
	struct node *next;	/* set before the node is published */
};

typedef struct {
	_Atomic(struct node *) top;
	_Atomic(uint64_t) slot;	/* state << 32 | item */
} mystack_t;

static uint64_t slot_make(unsigned int item, unsigned int state)
{
	return (uint64_t) state << 32 | item;
}
static unsigned int slot_item(uint64_t s) { return (unsigned int) s; }
static unsigned int slot_state(uint64_t s) { return (unsigned int) (s >> 32); }

void init_stack(mystack_t *s, int num_threads)
{
	atomic_init(&s->top, NULL);
	atomic_init(&s->slot, slot_make(0, EMPTY));
}

void clear_stack(mystack_t *s, int num_threads) {}

/* Exchanges my item; false if no partner was found (a timeout) */
static bool exchange(mystack_t *s, unsigned int mine, unsigned int *yours)
{
	uint64_t cur = atomic_load_explicit(&s->slot, memory_order_acquire);

	switch (slot_state(cur)) {
	case EMPTY:
		if (!atomic_compare_exchange_strong_explicit(&s->slot, &cur,
							     slot_make(mine, WAITING),
							     memory_order_acq_rel,
							     memory_order_acquire))
			return false;
		/* wait for a partner, once */
		cur = atomic_load_explicit(&s->slot, memory_order_acquire);
		if (slot_state(cur) == BUSY) {
			atomic_store_explicit(&s->slot, slot_make(0, EMPTY), memory_order_release);
			*yours = slot_item(cur);
			return true;
		}
		/* give up */
#ifdef BUG_TIMEOUT_STORE
		atomic_store_explicit(&s->slot, slot_make(0, EMPTY), memory_order_release);
		return false;
#else
		cur = slot_make(mine, WAITING);
		if (atomic_compare_exchange_strong_explicit(&s->slot, &cur, slot_make(0, EMPTY),
							    memory_order_acq_rel,
							    memory_order_acquire))
			return false;
		/* a partner arrived after all */
		atomic_store_explicit(&s->slot, slot_make(0, EMPTY), memory_order_release);
		*yours = slot_item(cur);
		return true;
#endif
	case WAITING:
		if (!atomic_compare_exchange_strong_explicit(&s->slot, &cur,
							     slot_make(mine, BUSY),
							     memory_order_acq_rel,
							     memory_order_acquire))
			return false;
		*yours = slot_item(cur);
		return true;
	default:
		return false;	/* BUSY: an exchange is finishing */
	}
}

void push(mystack_t *s, unsigned int value)
{
	struct node *n = malloc(sizeof(*n));

	n->value = value;
	for (;;) {
		struct node *top = atomic_load_explicit(&s->top, memory_order_relaxed);

		n->next = top;
		if (atomic_compare_exchange_strong_explicit(&s->top, &top, n, memory_order_acq_rel,
							    memory_order_relaxed))
			return;

		unsigned int other;

		/* eliminated by a pop (which exchanged the empty item) */
		if (exchange(s, value, &other) && other == 0)
			return;
	}
}

unsigned int pop(mystack_t *s)
{
	for (;;) {
		struct node *top = atomic_load_explicit(&s->top, memory_order_acquire);

		if (top == NULL)
			return 0;
		if (atomic_compare_exchange_strong_explicit(&s->top, &top, top->next,
							    memory_order_acq_rel,
							    memory_order_acquire))
			return top->value;

		unsigned int other;

		/* eliminated by a push */
		if (exchange(s, 0, &other) && other != 0)
			return other;
	}
}
