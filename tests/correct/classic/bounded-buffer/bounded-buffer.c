#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>
#include "../../../lib/sem.h"

/*
 * Producer-consumer with a finite buffer and semaphores (The Little Book of
 * Semaphores, 4.2; Java Concurrency in Practice, Listing 12.1): "spaces"
 * counts free slots, "items" counts full ones, and a mutex guards the
 * buffer. NPROD producers each put one item, NCONS consumers each take one;
 * every item is taken exactly once.
 *
 * One switch per bug:
 * - BUG_DEADLOCK: a consumer takes the mutex before waiting for an item
 *   (Deadlock #4, 4.1.3). It waits for an item holding the mutex, and no
 *   producer can get in to put one;
 * - BUG_NO_MUTEX: the buffer is not guarded, so producers write the same
 *   slot and index at once: a data race.
 */

#ifndef CAP
#define CAP 1
#endif
#define NPROD 2
#define NCONS 2

sema_t mutex = SEMA_INIT(1);
sema_t spaces = SEMA_INIT(CAP);
sema_t items;
int buf[CAP];		/* guarded by mutex */
int head, tail;
atomic_int consumed;	/* one bit per item */

static void lock(void)
{
#ifndef BUG_NO_MUTEX
	sema_wait(&mutex);
#endif
}

static void unlock(void)
{
#ifndef BUG_NO_MUTEX
	sema_signal(&mutex);
#endif
}

static void put(int item)
{
	sema_wait(&spaces);
	lock();
	buf[tail] = item;
	tail = (tail + 1) % CAP;
	unlock();
	sema_signal(&items);
}

static int take(void)
{
#ifdef BUG_DEADLOCK
	lock();
	sema_wait(&items);
#else
	sema_wait(&items);
	lock();
#endif
	int item = buf[head];
	head = (head + 1) % CAP;
	unlock();
	sema_signal(&spaces);
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

	assert(item != 0 && (seen & item) == 0);	/* a real item, taken once */
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
