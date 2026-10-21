/*
 * A retry loop around a wait loop: each iteration tries (state is 2 while
 * running), then waits, under the mutex, until the state or the generation
 * changes. Once the other thread sets state = 0, the wait loop exits and the
 * retry loop's next iteration exits too: -check-liveness must report
 * nothing, and the waiter must not be blocked in a retry iteration that it
 * would leave.
 *
 * The retry loop's iteration includes the iterations of the wait loop: it
 * is judged by all their reads (a stale state read in the retry part is
 * not repeated by the next iteration), not only by the last wait iteration
 * (which reads the final state and generation).
 */
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

pthread_mutex_t l = PTHREAD_MUTEX_INITIALIZER;
atomic_int state = 2, gen;

void *waiter(void *unused)
{
	for (;;) {
		int s;
		pthread_mutex_lock(&l);
		s = atomic_load_explicit(&state, memory_order_relaxed);
		pthread_mutex_unlock(&l);
		if (s != 2)
			break;
		for (;;) {
			int st, g;
			pthread_mutex_lock(&l);
			st = atomic_load_explicit(&state, memory_order_relaxed);
			g = atomic_load_explicit(&gen, memory_order_relaxed);
			pthread_mutex_unlock(&l);
			if (st != 2)
				break;
			__VERIFIER_await(atomic_load_explicit(&gen, memory_order_relaxed) != g);
		}
	}
	return NULL;
}

void *stopper(void *unused)
{
	pthread_mutex_lock(&l);
	atomic_store_explicit(&state, 0, memory_order_relaxed);
	atomic_store_explicit(&gen, atomic_load_explicit(&gen, memory_order_relaxed) + 1,
			      memory_order_relaxed);
	pthread_mutex_unlock(&l);
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	pthread_create(&t1, NULL, waiter, NULL);
	pthread_create(&t2, NULL, stopper, NULL);
	return 0;
}
