#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>
#include "../../../lib/lock.h"
#include "../../../lib/cond.h"

/*
 * A bounded buffer with a lock and two condition variables (The Art of
 * Multiprocessor Programming, Fig. 8.5; Java Concurrency in Practice,
 * Listing 14.11). put() waits while the buffer is full, take() while it is
 * empty; each signals the other side. NPROD producers each put one item,
 * NCONS consumers each take one; every item is taken exactly once.
 *
 * One switch per bug:
 * - BUG_IF: take() checks for an empty buffer with "if" instead of "while".
 *   A woken consumer must take the lock again, and another consumer can
 *   take the item first: the woken one then takes from an empty buffer;
 * - BUG_LOST_WAKEUP: put() signals only when the buffer was empty (Fig. 8.6).
 *   With two consumers waiting, two puts in a row wake only one of them,
 *   and the other waits forever although an item is there (-check-liveness).
 */

#ifndef CAP
#define CAP 2
#endif
#define NPROD 2
#define NCONS 2

lock_t lock;
cond_t not_empty;
cond_t not_full;
int buf[CAP];		/* guarded by lock */
int head, tail, count;
atomic_int consumed;	/* one bit per item */

static void put(int item)
{
	lock_acquire(&lock);
	while (count == CAP)
		cond_wait(&not_full, &lock);
	buf[tail] = item;
	tail = (tail + 1) % CAP;
#ifdef BUG_LOST_WAKEUP
	if (++count == 1)
		cond_signal(&not_empty);
#else
	++count;
	cond_signal(&not_empty);
#endif
	lock_release(&lock);
}

static int take(void)
{
	lock_acquire(&lock);
#ifdef BUG_IF
	if (count == 0)
		cond_wait(&not_empty, &lock);
#else
	while (count == 0)
		cond_wait(&not_empty, &lock);
#endif
	assert(count > 0);
	int item = buf[head];
	head = (head + 1) % CAP;
	--count;
	cond_signal(&not_full);
	lock_release(&lock);
	return item;
}

static void *producer(void *arg)
{
	put(1 << (intptr_t) arg);
	return NULL;
}

static void *consumer(void *unused)
{
	int item = take();
	int seen = atomic_fetch_or_explicit(&consumed, item, memory_order_relaxed);

	assert((seen & item) == 0);	/* taken once */
	return NULL;
}

int main()
{
	pthread_t p[NPROD], c[NCONS];

	for (intptr_t i = 0; i < NPROD; i++)
		if (pthread_create(&p[i], NULL, producer, (void *) i))
			abort();
	for (int i = 0; i < NCONS; i++)
		if (pthread_create(&c[i], NULL, consumer, NULL))
			abort();
	for (int i = 0; i < NPROD; i++)
		if (pthread_join(p[i], NULL))
			abort();
	for (int i = 0; i < NCONS; i++)
		if (pthread_join(c[i], NULL))
			abort();
	assert(consumed == (1 << NPROD) - 1);
	return 0;
}
