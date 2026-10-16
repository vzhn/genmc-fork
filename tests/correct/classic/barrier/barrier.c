#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>
#include "../../../lib/sem.h"

/*
 * A one-shot barrier (The Little Book of Semaphores, 3.6) for N threads: no
 * thread passes it before all N have arrived. Arrivals are counted under a
 * mutex; the last thread to arrive opens a turnstile, and each thread that
 * passes the turnstile lets the next one through.
 *
 * One switch per bug:
 * - BUG_NO_TURNSTILE: the last thread signals once and nobody passes the
 *   signal on (the non-solution of 3.6.2): only one thread gets through;
 * - BUG_WAIT_IN_MUTEX: the turnstile is inside the mutex (Deadlock #2,
 *   3.6.3): the first thread waits at the turnstile holding the mutex, so
 *   nobody else can arrive.
 */

#ifndef N
#define N 3
#endif

sema_t mutex = SEMA_INIT(1);
sema_t barrier;
int count;		/* guarded by mutex */
atomic_bool arrived[N];

static void *thread(void *arg)
{
	atomic_store_explicit(&arrived[(intptr_t) arg], true, memory_order_relaxed);

#ifdef BUG_WAIT_IN_MUTEX
	sema_wait(&mutex);
	if (++count == N)
		sema_signal(&barrier);
	sema_wait(&barrier);
	sema_signal(&barrier);
	sema_signal(&mutex);
#else
	sema_wait(&mutex);
	int last = ++count == N;
	sema_signal(&mutex);

	if (last)
		sema_signal(&barrier);
	sema_wait(&barrier);
# ifndef BUG_NO_TURNSTILE
	sema_signal(&barrier);
# endif
#endif

	/* past the barrier: everyone has arrived */
	for (int i = 0; i < N; i++)
		assert(atomic_load_explicit(&arrived[i], memory_order_relaxed));
	return NULL;
}

int main()
{
	pthread_t t[N];

	for (intptr_t i = 0; i < N; i++)
		if (pthread_create(&t[i], NULL, thread, (void *) i))
			abort();
	for (int i = 0; i < N; i++)
		if (pthread_join(t[i], NULL))
			abort();
	return 0;
}
