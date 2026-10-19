#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * Dmitry Vyukov's intrusive MPSC node-based queue (1024cores.net), used in
 * actor runtimes and in Rust's old std::sync::mpsc: producers swap the head
 * with an exchange and then link the previous head to their node; the
 * single consumer follows next pointers from the tail, starting at a stub
 * node. A producer stalled between its exchange and its link hides the
 * nodes behind it: pop() then reports "inconsistent" (the queue is not
 * empty, but cannot be popped yet), and the consumer waits for the link.
 * Payloads are plain memory, published by the link (a release store).
 *
 * NPROD producers each write a value into their node and push it; the
 * consumer pops until it has all of them, waiting (an await) when the queue
 * is empty, or inconsistent until a node is linked or the head moves. Each
 * value is popped exactly once.
 *
 * BUG_RELAXED_LINK: the producer links with a relaxed store, a common
 * porting mistake (the exchange is acq_rel, but the link is what the
 * consumer reads). The consumer's read of the node's payload is not
 * ordered after the producer's write: a data race. Passes under SC.
 */

#ifndef NPROD
# define NPROD 2
#endif

#ifdef BUG_RELAXED_LINK
# define LINK_STORE memory_order_relaxed
#else
# define LINK_STORE memory_order_release
#endif

struct node {
	_Atomic(struct node *) next;
	int value;
};

_Atomic(struct node *) head;	/* producers push here */
struct node *tail;		/* the consumer's end */
struct node stub;
struct node nodes[NPROD];

#define EMPTY 0
#define INCONSISTENT -1

static void push(struct node *n)
{
	atomic_store_explicit(&n->next, NULL, memory_order_relaxed);
	struct node *prev = atomic_exchange_explicit(&head, n, memory_order_acq_rel);
	/* a producer stalled here hides n and everything after it */
	atomic_store_explicit(&prev->next, n, LINK_STORE);
}

struct node *seen_head;		/* the head pop() saw when inconsistent */

/* Returns a value, EMPTY or INCONSISTENT */
static int pop(void)
{
	struct node *t = tail;
	struct node *next = atomic_load_explicit(&t->next, memory_order_acquire);

	if (t == &stub) {
		if (next == NULL)
			return EMPTY;
		tail = next;
		t = next;
		next = atomic_load_explicit(&next->next, memory_order_acquire);
	}
	if (next) {
		tail = next;
		return t->value;
	}
	seen_head = atomic_load_explicit(&head, memory_order_acquire);
	if (t != seen_head)
		return INCONSISTENT;
	push(&stub);
	next = atomic_load_explicit(&t->next, memory_order_acquire);
	if (next) {
		tail = next;
		return t->value;
	}
	return INCONSISTENT;
}

static void *producer(void *arg)
{
	struct node *n = &nodes[(intptr_t) arg];

	n->value = (intptr_t) arg + 1;
	push(n);
	return NULL;
}

static void *consumer(void *unused)
{
	int seen[NPROD + 1] = { 0 };

	for (int got = 0; got < NPROD;) {
		int v = pop();

		if (v > 0) {
			assert(v <= NPROD && seen[v] == 0);
			seen[v]++;
			got++;
			continue;
		}
		/* wait until a node is linked or, if inconsistent, the head moves */
		struct node *t = tail;

		if (v == EMPTY)
			__VERIFIER_await(atomic_load_explicit(&t->next, memory_order_relaxed) != NULL);
		else
			__VERIFIER_await(atomic_load_explicit(&t->next, memory_order_relaxed) != NULL ||
					 atomic_load_explicit(&head, memory_order_relaxed) != seen_head);
	}
	return NULL;
}

int main()
{
	pthread_t p[NPROD], c;

	atomic_init(&stub.next, NULL);
	atomic_init(&head, &stub);
	tail = &stub;

	for (intptr_t i = 0; i < NPROD; i++)
		if (pthread_create(&p[i], NULL, producer, (void *) i))
			abort();
	if (pthread_create(&c, NULL, consumer, NULL))
		abort();
	for (int i = 0; i < NPROD; i++)
		if (pthread_join(p[i], NULL))
			abort();
	if (pthread_join(c, NULL))
		abort();
	return 0;
}
