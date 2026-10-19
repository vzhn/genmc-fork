#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * The unbounded channel of crossbeam-channel (flavors/list.rs; Rust std's
 * mpmc has the same code): a linked list of blocks of slots, with a head
 * and a tail position, each an index and a block pointer. Both block
 * pointers start null; the first sender allocates the first block, installs
 * it in the tail with a CAS and then stores it in the head. When the last
 * receiver is dropped, disconnect_receivers() marks the tail and
 * discard_all_messages() drops the queued messages and frees the blocks;
 * when the last sender is dropped too, Channel::drop() frees what is left.
 *
 * The model keeps what this needs: one block (never filled), integer
 * messages, and the senders' and receiver's drops done by hand.
 * Thread A sends a message, then drops the only receiver; thread B sends a
 * message; main drops the channel once both are done. Waits are awaits.
 *
 * BUG_LOAD_IN_LOOP: crossbeam-channel 0.5.12 to 0.5.14 and Rust 1.78 to 1.86
 * (RUSTSEC-2025-0024, CVE-2025-4574). discard_all_messages() takes the head
 * block with a swap that leaves null behind, but if that finds no block yet
 * (B installed it in the tail but not yet in the head) while messages are
 * queued, it waits for the block with a plain load. It then frees the block
 * and leaves the head pointing to it, and Channel::drop() frees it again: a
 * double free. Fixed in 0.5.15 by swapping in the loop as well.
 */

#define SHIFT 1
#define MARK_BIT 1UL
#define LAP 4
#define BLOCK_CAP (LAP - 1)
#define WRITE 1

struct slot {
	int msg;
	atomic_int state;
};

struct block {
	struct slot slots[BLOCK_CAP];
};

struct position {
	atomic_ulong index;
	_Atomic(struct block *) block;
};

struct channel {
	struct position head;
	struct position tail;
};

struct channel ch;

struct token {
	struct block *block;
	unsigned long offset;
};

/* Returns false if the channel is disconnected */
static bool start_send(struct token *tok)
{
	unsigned long tail = atomic_load_explicit(&ch.tail.index, memory_order_acquire);
	struct block *block = atomic_load_explicit(&ch.tail.block, memory_order_acquire);

	for (;;) {
		if (tail & MARK_BIT)
			return false;

		unsigned long offset = (tail >> SHIFT) % LAP;

		assert(offset < BLOCK_CAP);	/* the model never fills a block */

		/* the first message: allocate the first block and install it */
		if (block == NULL) {
			struct block *new = calloc(1, sizeof(*new));
			struct block *expected = NULL;

			if (atomic_compare_exchange_strong_explicit(&ch.tail.block, &expected, new,
								    memory_order_release,
								    memory_order_relaxed)) {
				atomic_store_explicit(&ch.head.block, new, memory_order_release);
				block = new;
			} else {
				free(new);
				tail = atomic_load_explicit(&ch.tail.index, memory_order_acquire);
				block = atomic_load_explicit(&ch.tail.block, memory_order_acquire);
				continue;
			}
		}

		if (atomic_compare_exchange_strong_explicit(&ch.tail.index, &tail,
							    tail + (1 << SHIFT),
							    memory_order_seq_cst,
							    memory_order_acquire)) {
			tok->block = block;
			tok->offset = offset;
			return true;
		}
		block = atomic_load_explicit(&ch.tail.block, memory_order_acquire);
	}
}

static void send(int msg)
{
	struct token tok;

	if (!start_send(&tok))
		return;
	struct slot *s = &tok.block->slots[tok.offset];
	s->msg = msg;
	atomic_fetch_or_explicit(&s->state, WRITE, memory_order_release);
}

static void discard_all_messages(void)
{
	unsigned long tail = atomic_load_explicit(&ch.tail.index, memory_order_acquire);
	unsigned long head = atomic_load_explicit(&ch.head.index, memory_order_acquire);
	struct block *block = atomic_exchange_explicit(&ch.head.block, NULL, memory_order_acq_rel);

	/* messages are queued but the first block is still being installed */
	if (head >> SHIFT != tail >> SHIFT) {
		while (block == NULL) {
			__VERIFIER_await(atomic_load_explicit(&ch.head.block,
							      memory_order_relaxed) != NULL);
#ifdef BUG_LOAD_IN_LOOP
			block = atomic_load_explicit(&ch.head.block, memory_order_acquire);
#else
			block = atomic_exchange_explicit(&ch.head.block, NULL, memory_order_acq_rel);
#endif
		}
	}

	/* drop the queued messages (wait for each to be written) */
	for (; head >> SHIFT != tail >> SHIFT; head += 1 << SHIFT) {
		struct slot *s = &block->slots[(head >> SHIFT) % LAP];

		__VERIFIER_await(atomic_load_explicit(&s->state, memory_order_acquire) & WRITE);
		assert(s->msg == 1 || s->msg == 2);
	}
	free(block);

	head &= ~MARK_BIT;
	atomic_store_explicit(&ch.head.index, head, memory_order_release);
}

/* The last receiver is dropped */
static void disconnect_receivers(void)
{
	unsigned long tail = atomic_fetch_or_explicit(&ch.tail.index, MARK_BIT,
						      memory_order_seq_cst);

	if (!(tail & MARK_BIT))
		discard_all_messages();
}

/* Both sides are gone (Channel::drop) */
static void channel_drop(void)
{
	unsigned long head = atomic_load_explicit(&ch.head.index, memory_order_relaxed);
	unsigned long tail = atomic_load_explicit(&ch.tail.index, memory_order_relaxed);
	struct block *block = atomic_load_explicit(&ch.head.block, memory_order_relaxed);

	/* the receiver has discarded every message */
	assert(head >> SHIFT == tail >> SHIFT);
	free(block);
}

static void *thread_a(void *unused)
{
	send(1);
	disconnect_receivers();
	return NULL;
}

static void *thread_b(void *unused)
{
	send(2);
	return NULL;
}

int main()
{
	pthread_t a, b;

	if (pthread_create(&a, NULL, thread_a, NULL))
		abort();
	if (pthread_create(&b, NULL, thread_b, NULL))
		abort();
	if (pthread_join(a, NULL))
		abort();
	if (pthread_join(b, NULL))
		abort();
	channel_drop();
	return 0;
}
