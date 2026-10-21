/*
 * The outer loop's iteration writes x (in its first two iterations), then
 * runs an inner spinloop, which exits at once. A second iteration, which
 * reads done == 1 and makes x == 2, is reachable. The outer loop's iteration
 * includes the inner loop's: it has a side effect although the inner loop's
 * last iteration (all that follows the inner loop's last spin start) has
 * none, and it must not be pruned.
 *
 * LOCKED: the inner loop takes a mutex (a loop checked at run time, even
 * without -check-liveness).
 */
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

pthread_mutex_t l = PTHREAD_MUTEX_INITIALIZER;
atomic_int x, flag = 1, done;

void *thread_1(void *unused)
{
	for (;;) {
		if (atomic_load_explicit(&x, memory_order_relaxed) < 2)
			atomic_store_explicit(&x, atomic_load_explicit(&x, memory_order_relaxed) + 1,
					      memory_order_relaxed);
#ifdef LOCKED
		for (;;) {
			int f;
			pthread_mutex_lock(&l);
			f = atomic_load_explicit(&flag, memory_order_relaxed);
			pthread_mutex_unlock(&l);
			if (f)
				break;
		}
#else
		while (!atomic_load_explicit(&flag, memory_order_relaxed))
			;
#endif
		if (atomic_load_explicit(&done, memory_order_relaxed))
			break;
	}
	return NULL;
}

void *thread_2(void *unused)
{
	atomic_store_explicit(&done, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	pthread_create(&t1, NULL, thread_1, NULL);
	pthread_create(&t2, NULL, thread_2, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	assert(atomic_load_explicit(&x, memory_order_relaxed) != 2);
	return 0;
}
