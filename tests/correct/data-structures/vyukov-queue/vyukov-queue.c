#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <assert.h>

#ifndef QUEUE_CAP
# define QUEUE_CAP 2
#endif
#include "my_queue.c"

/*
 * A client of Vyukov's bounded MPMC queue (my_queue.c): NPROD producers
 * enqueue one value each and NCONS consumers dequeue once each; afterwards
 * main drains the queue. Every value comes out exactly once. Before the
 * threads start, main enqueues and dequeues one value, so the positions the
 * threads use wrap around the two-cell ring and reuse a cell from the
 * previous lap.
 */

#ifndef NPROD
# define NPROD 2
#endif
#ifndef NCONS
# define NCONS 1
#endif

queue_t q;
atomic_int seen[NPROD + 1];

static void got(unsigned int v)
{
	assert(v >= 1 && v <= NPROD);
	int before = atomic_fetch_add_explicit(&seen[v], 1, memory_order_relaxed);
	assert(before == 0);
}

static void *producer(void *arg)
{
	bool ok = enqueue(&q, (intptr_t) arg);

	assert(ok);
	return NULL;
}

static void *consumer(void *unused)
{
	unsigned int v;

	if (dequeue(&q, &v))
		got(v);
	return NULL;
}

int main()
{
	pthread_t p[NPROD], c[NCONS];
	unsigned int v;

	init_queue(&q, NPROD + NCONS);
	enqueue(&q, 0);
	dequeue(&q, &v);

	for (intptr_t i = 0; i < NPROD; i++)
		if (pthread_create(&p[i], NULL, producer, (void *) (i + 1)))
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

	while (dequeue(&q, &v))
		got(v);
	for (int i = 1; i <= NPROD; i++)
		assert(seen[i] == 1);
	return 0;
}
