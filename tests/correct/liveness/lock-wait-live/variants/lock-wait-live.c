/*
 * Threads that wait in pthread_mutex_lock() or a condition-variable wait
 * and are eventually released: -check-liveness must report nothing.
 *
 * CONTENDED: two threads take the same mutex.
 * HOLDER_SPINS: the holder spins, under the lock, on a flag another thread
 *	sets; a third thread waits for the lock.
 * CONDVAR_LOOP: a waiter loops on a flag around a condition-variable wait;
 *	the signaller sets the flag and signals. A wait that succeeds and is
 *	followed by a stale flag read is a spinloop iteration, judged by all its
 *	reads, not by the wait's read alone.
 * ZNE_HOLDER: a lock/unlock loop that polls a flag under the lock (a lock
 *	ZNE spinloop, blocked between its lock and its unlock) holds the lock
 *	that another thread waits for; that thread is not stuck.
 */
#include <pthread.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <genmc.h>

pthread_mutex_t a;
__VERIFIER_cond_t c;
atomic_int flag;

void *thread_1(void *unused)
{
#if defined(CONTENDED)
	pthread_mutex_lock(&a);
	pthread_mutex_unlock(&a);
#elif defined(HOLDER_SPINS)
	pthread_mutex_lock(&a);
	while (!atomic_load_explicit(&flag, memory_order_acquire))
		;
	pthread_mutex_unlock(&a);
#elif defined(CONDVAR_LOOP)
	while (!atomic_load_explicit(&flag, memory_order_acquire))
		__VERIFIER_cond_wait(&c);
#elif defined(ZNE_HOLDER)
	for (;;) {
		pthread_mutex_lock(&a);
		if (atomic_load_explicit(&flag, memory_order_relaxed)) {
			pthread_mutex_unlock(&a);
			break;
		}
		pthread_mutex_unlock(&a);
	}
#endif
	return NULL;
}

void *thread_2(void *unused)
{
#if defined(CONTENDED) || defined(HOLDER_SPINS)
	pthread_mutex_lock(&a);
	pthread_mutex_unlock(&a);
#elif defined(CONDVAR_LOOP)
	atomic_store_explicit(&flag, 1, memory_order_release);
	__VERIFIER_cond_signal(&c);
#elif defined(ZNE_HOLDER)
	pthread_mutex_lock(&a);
	atomic_store_explicit(&flag, 1, memory_order_relaxed);
	pthread_mutex_unlock(&a);
#endif
	return NULL;
}

void *thread_3(void *unused)
{
#if defined(HOLDER_SPINS)
	atomic_store_explicit(&flag, 1, memory_order_release);
#endif
	return NULL;
}

int main()
{
	pthread_t t1, t2, t3;

	pthread_mutex_init(&a, NULL);
	__VERIFIER_cond_init(&c, NULL);
	if (pthread_create(&t1, NULL, thread_1, NULL))
		abort();
	if (pthread_create(&t2, NULL, thread_2, NULL))
		abort();
	if (pthread_create(&t3, NULL, thread_3, NULL))
		abort();
	return 0;
}
