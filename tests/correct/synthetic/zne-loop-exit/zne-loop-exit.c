/*
 * A spinloop whose iteration has zero net effect (a lock/unlock pair, LOCK,
 * or an increment cancelled by a decrement, FAI) and exits after it, on a
 * flag read earlier in the iteration. Spin-assume blocks such an iteration
 * before the unlock (the decrement); it blocked the exiting iteration too,
 * so no execution got past the loop (0 complete executions) and an error
 * after the loop was missed. DO_WHILE: the flag is read in the critical
 * section, and the loop's exit test comes after the unlock.
 */

#include <pthread.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <assert.h>

pthread_mutex_t a;
atomic_int flag;
atomic_int cnt;

void *thread_1(void *unused)
{
#ifdef DO_WHILE
	int f;

	do {
		pthread_mutex_lock(&a);
		f = atomic_load_explicit(&flag, memory_order_relaxed);
		pthread_mutex_unlock(&a);
	} while (!f);
#else
	for (;;) {
		int f = atomic_load_explicit(&flag, memory_order_acquire);
#ifdef LOCK
		pthread_mutex_lock(&a);
		pthread_mutex_unlock(&a);
#else
		atomic_fetch_add_explicit(&cnt, 1, memory_order_relaxed);
		atomic_fetch_sub_explicit(&cnt, 1, memory_order_relaxed);
#endif
		if (f)
			break;
	}
#endif
#ifdef AFTER_LOOP
	AFTER_LOOP;
#endif
	return NULL;
}

void *thread_2(void *unused)
{
#ifdef LOCK
	pthread_mutex_lock(&a);
#endif
	atomic_store_explicit(&flag, 1, memory_order_release);
#ifdef LOCK
	pthread_mutex_unlock(&a);
#endif
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	pthread_mutex_init(&a, NULL);
	if (pthread_create(&t1, NULL, thread_1, NULL))
		abort();
	if (pthread_create(&t2, NULL, thread_2, NULL))
		abort();
	return 0;
}
