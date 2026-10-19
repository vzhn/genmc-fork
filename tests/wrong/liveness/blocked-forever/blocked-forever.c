/*
 * Threads blocked forever in pthread_mutex_lock() or a condition-variable
 * wait. Both block on a single read, a spinloop without loop markers, and
 * -check-liveness used to walk back from it looking for the loop's start and
 * crash (SIGILL).
 *
 * ABBA: the two threads take the locks in opposite orders.
 * EXIT_LOCKED: one thread exits holding the lock.
 * NO_SIGNAL: a thread waits on a condition variable nobody signals. The wait
 * may return spuriously (it does under IMM); the thread then spins forever.
 * ZNE_OTHER: the holder of a spins forever in a lock/unlock loop on b (a lock
 *	ZNE spinloop, blocked between its lock of b and its unlock).
 * FAI_HOLDER: the holder of a spins forever in an increment/decrement loop
 *	(an FAI ZNE spinloop).
 */

#include <pthread.h>
#include <stdlib.h>
#include <genmc.h>
#include <stdatomic.h>

pthread_mutex_t a;
pthread_mutex_t b;
__VERIFIER_cond_t c;
atomic_int ready;
atomic_int cnt;

void *thread_1(void *unused)
{
	pthread_mutex_lock(&a);
#ifdef ZNE_OTHER
	for (;;) {
		pthread_mutex_lock(&b);
		if (atomic_load_explicit(&ready, memory_order_relaxed)) {
			pthread_mutex_unlock(&b);
			break;
		}
		pthread_mutex_unlock(&b);
	}
#endif
#ifdef FAI_HOLDER
	for (;;) {
		if (atomic_fetch_add_explicit(&cnt, 1, memory_order_relaxed) == 5)
			break;
		atomic_fetch_sub_explicit(&cnt, 1, memory_order_relaxed);
	}
#endif
#ifdef ABBA
	pthread_mutex_lock(&b);
	pthread_mutex_unlock(&b);
#endif
	pthread_mutex_unlock(&a);
	return NULL;
}

void *thread_2(void *unused)
{
#ifdef ABBA
	pthread_mutex_lock(&b);
	pthread_mutex_lock(&a);
	pthread_mutex_unlock(&a);
	pthread_mutex_unlock(&b);
#endif
#ifdef EXIT_LOCKED
	pthread_mutex_lock(&a);
#endif
#if defined(ZNE_OTHER) || defined(FAI_HOLDER)
	pthread_mutex_lock(&a);
	pthread_mutex_unlock(&a);
#endif
#ifdef NO_SIGNAL
	__VERIFIER_cond_wait(&c);
	while (!atomic_load_explicit(&ready, memory_order_relaxed))
		;
#endif
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	pthread_mutex_init(&a, NULL);
	pthread_mutex_init(&b, NULL);
	__VERIFIER_cond_init(&c, NULL);
	if (pthread_create(&t1, NULL, thread_1, NULL))
		abort();
	if (pthread_create(&t2, NULL, thread_2, NULL))
		abort();
	return 0;
}
