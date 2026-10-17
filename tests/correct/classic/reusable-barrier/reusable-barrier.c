#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>
#include "../../../lib/sem.h"

/*
 * A reusable barrier (The Little Book of Semaphores, 3.7): N threads pass it
 * ROUNDS times, and in no round does a thread pass before all N have arrived
 * in that round.
 *
 * Without a switch: the two-turnstile solution (3.7.5). The last thread to
 * arrive locks the second turnstile and opens the first; the last to leave
 * locks the first and opens the second. PRELOADED selects the other
 * solution (3.7.6), where the last thread signals a turnstile N times.
 *
 * One switch per bug:
 * - BUG_NOSOL1: the count is checked outside the mutex (non-solution #1,
 *   3.7.1). Several threads can see count == N and open the turnstile
 *   more than once, so it lets a thread through early in the next round;
 * - BUG_NOSOL2: one turnstile, the count checked under the mutex
 *   (non-solution #2, 3.7.3). A thread can leave, loop around and pass the
 *   turnstile of the next round before the others arrive.
 */

#ifndef N
#define N 2
#endif
#ifndef ROUNDS
#define ROUNDS 2
#endif

sema_t mutex = SEMA_INIT(1);
sema_t turnstile;
#ifdef PRELOADED
sema_t turnstile2;
#else
sema_t turnstile2 = SEMA_INIT(1);
#endif
atomic_int count;		/* updated under mutex */
atomic_bool arrived[ROUNDS][N];

/* count += d, under the mutex */
static int add_count(int d)
{
	int c = atomic_load_explicit(&count, memory_order_relaxed) + d;

	atomic_store_explicit(&count, c, memory_order_relaxed);
	return c;
}

static int get_count(void)
{
	return atomic_load_explicit(&count, memory_order_relaxed);
}

static void signal_n(sema_t *s, int n)
{
	for (int i = 0; i < n; i++)
		sema_signal(s);
}

static void enter(void)
{
#if defined(BUG_NOSOL1)
	sema_wait(&mutex);
	add_count(1);
	sema_signal(&mutex);
	if (get_count() == N)
		sema_signal(&turnstile);
	sema_wait(&turnstile);
	sema_signal(&turnstile);
#elif defined(BUG_NOSOL2)
	sema_wait(&mutex);
	if (add_count(1) == N)
		sema_signal(&turnstile);
	sema_signal(&mutex);
	sema_wait(&turnstile);
	sema_signal(&turnstile);
#elif defined(PRELOADED)
	sema_wait(&mutex);
	if (add_count(1) == N)
		signal_n(&turnstile, N);
	sema_signal(&mutex);
	sema_wait(&turnstile);
#else
	sema_wait(&mutex);
	if (add_count(1) == N) {
		sema_wait(&turnstile2);
		sema_signal(&turnstile);
	}
	sema_signal(&mutex);
	sema_wait(&turnstile);
	sema_signal(&turnstile);
#endif
}

static void leave(void)
{
#if defined(BUG_NOSOL1)
	sema_wait(&mutex);
	add_count(-1);
	sema_signal(&mutex);
	if (get_count() == 0)
		sema_wait(&turnstile);
#elif defined(BUG_NOSOL2)
	sema_wait(&mutex);
	if (add_count(-1) == 0)
		sema_wait(&turnstile);
	sema_signal(&mutex);
#elif defined(PRELOADED)
	sema_wait(&mutex);
	if (add_count(-1) == 0)
		signal_n(&turnstile2, N);
	sema_signal(&mutex);
	sema_wait(&turnstile2);
#else
	sema_wait(&mutex);
	if (add_count(-1) == 0) {
		sema_wait(&turnstile);
		sema_signal(&turnstile2);
	}
	sema_signal(&mutex);
	sema_wait(&turnstile2);
	sema_signal(&turnstile2);
#endif
}

static void *thread(void *arg)
{
	int me = (intptr_t) arg;

	for (int r = 0; r < ROUNDS; r++) {
		atomic_store_explicit(&arrived[r][me], true, memory_order_relaxed);
		enter();
		/* critical point: everyone has arrived in this round */
		for (int i = 0; i < N; i++)
			assert(atomic_load_explicit(&arrived[r][i], memory_order_relaxed));
		leave();
	}
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
