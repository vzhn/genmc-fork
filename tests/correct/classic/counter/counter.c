#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * A shared counter (Java Concurrency in Practice, Listings 1.1, 2.2 and
 * 15.2): N threads each increment it once, and at the end it is N.
 *
 * Without a switch the increment is one atomic fetch-and-add. The N
 * increments can be ordered in N! ways and nothing else varies, so GenMC
 * explores exactly N! executions under every model: an execution count
 * that is known independently of GenMC.
 *
 * One switch per bug:
 * - BUG_PLAIN: count++ on a plain int: a data race;
 * - BUG_LOAD_STORE: an atomic load, then an atomic store of the value + 1.
 *   No race, but two threads can read the same value and an increment is
 *   lost.
 */

#ifndef N
#define N 3
#endif

#ifdef BUG_PLAIN
int count;
#else
atomic_int count;
#endif

static void *thread(void *unused)
{
#if defined(BUG_PLAIN)
	count++;
#elif defined(BUG_LOAD_STORE)
	int c = atomic_load_explicit(&count, memory_order_relaxed);
	atomic_store_explicit(&count, c + 1, memory_order_relaxed);
#else
	atomic_fetch_add_explicit(&count, 1, memory_order_relaxed);
#endif
	return NULL;
}

int main()
{
	pthread_t t[N];

	for (int i = 0; i < N; i++)
		if (pthread_create(&t[i], NULL, thread, NULL))
			abort();
	for (int i = 0; i < N; i++)
		if (pthread_join(t[i], NULL))
			abort();
	assert(count == N);
	return 0;
}
