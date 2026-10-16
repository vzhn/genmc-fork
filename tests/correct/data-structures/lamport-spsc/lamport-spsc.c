#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * Lamport's single-producer single-consumer ring buffer in C11, with the
 * orderings of Lê, Pop, Cohen and Zappa Nardelli (Correct and Efficient
 * Bounded FIFO Queues, SBAC-PAD 2013). The producer owns back, the
 * consumer owns front, and the slots are plain memory. A push checks for
 * room by loading front with acquire (so the consumer's read of the slot it
 * reuses is done), writes the slot and publishes it by storing back with
 * release; a pop loads back with acquire, reads the slot and frees it by
 * storing front with release. Each side reads its own index relaxed.
 *
 * The producer pushes NITEMS values into a ring of CAP slots, so slots are
 * reused; the consumer pops them, in order. A full or empty ring is waited
 * out with an await.
 *
 * One switch per bug:
 * - BUG_RELAXED_BACK: the pop loads back relaxed. It can see the new back
 *   before the slot's contents: its read of the slot races with the write;
 * - BUG_RELAXED_FRONT: the push loads front relaxed, a common mistake when
 *   only the producer-to-consumer direction is synchronized. The producer
 *   can reuse a slot while the consumer is still reading it: the write
 *   races with the read.
 */

#define CAP 2
#define NITEMS 3

#ifdef BUG_RELAXED_BACK
# define BACK_LOAD memory_order_relaxed
#else
# define BACK_LOAD memory_order_acquire
#endif
#ifdef BUG_RELAXED_FRONT
# define FRONT_LOAD memory_order_relaxed
#else
# define FRONT_LOAD memory_order_acquire
#endif

atomic_ulong front;	/* written by the consumer */
atomic_ulong back;	/* written by the producer */
int slots[CAP];

static void push(int v)
{
	unsigned long b = atomic_load_explicit(&back, memory_order_relaxed);

	/* wait for room */
	__VERIFIER_await(b - atomic_load_explicit(&front, FRONT_LOAD) < CAP);
	slots[b % CAP] = v;
	atomic_store_explicit(&back, b + 1, memory_order_release);
}

static int pop(void)
{
	unsigned long f = atomic_load_explicit(&front, memory_order_relaxed);

	/* wait for a value */
	__VERIFIER_await(atomic_load_explicit(&back, BACK_LOAD) != f);
	int v = slots[f % CAP];
	atomic_store_explicit(&front, f + 1, memory_order_release);
	return v;
}

static void *producer(void *unused)
{
	for (int i = 1; i <= NITEMS; i++)
		push(i);
	return NULL;
}

static void *consumer(void *unused)
{
	for (int i = 1; i <= NITEMS; i++)
		assert(pop() == i);
	return NULL;
}

int main()
{
	pthread_t p, c;

	if (pthread_create(&p, NULL, producer, NULL))
		abort();
	if (pthread_create(&c, NULL, consumer, NULL))
		abort();
	if (pthread_join(p, NULL))
		abort();
	if (pthread_join(c, NULL))
		abort();
	return 0;
}
