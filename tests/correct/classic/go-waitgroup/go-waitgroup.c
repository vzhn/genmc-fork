#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>
#include "../../../lib/sem.h"

/*
 * Go's sync.WaitGroup (src/sync/waitgroup.go): one 64-bit state word holds
 * the counter (high 32 bits) and the number of waiters (low 32 bits). Add
 * adds to the counter; when it drops to zero with waiters, it resets the
 * state and releases each waiter on the semaphore. Wait returns at once if
 * the counter is zero, and otherwise registers as a waiter with a CAS and
 * sleeps on the semaphore. Go's atomics are seq_cst; the runtime semaphore
 * is a sema_t.
 *
 * main starts two workers, waits for them, and then reads the results they
 * wrote, which must all be there. Without a switch main calls Add(1) for
 * each worker before starting it, as the documentation requires ("calls
 * with a positive delta that occur when the counter is zero must happen
 * before a Wait").
 *
 * BUG_ADD_IN_GOROUTINE: each worker calls Add(1) itself, the common mistake
 * staticcheck reports as SA2000. Wait can find the counter still zero and
 * return before the workers have started: main reads results that are not
 * written yet (or that a worker is writing, a data race).
 */

#define NWORKERS 2

atomic_ulong state;	/* counter << 32 | waiters */
sema_t sema;
int result[NWORKERS];

static void wg_add(long delta)
{
	unsigned long s = atomic_fetch_add(&state, (unsigned long) delta << 32) +
			  ((unsigned long) delta << 32);
	long v = (long) (s >> 32);
	unsigned int w = (unsigned int) s;

	assert(v >= 0);		/* "sync: negative WaitGroup counter" */
	if (v > 0 || w == 0)
		return;
	/* the counter is zero with waiters: release them all */
	atomic_store(&state, 0);
	for (unsigned int i = 0; i < w; i++)
		sema_signal(&sema);
}

static void wg_done(void)
{
	wg_add(-1);
}

static void wg_wait(void)
{
	for (;;) {
		unsigned long s = atomic_load(&state);

		if ((s >> 32) == 0)
			return;
		if (atomic_compare_exchange_strong(&state, &s, s + 1)) {
			sema_wait(&sema);
			return;
		}
	}
}

static void *worker(void *arg)
{
	int me = (intptr_t) arg;

#ifdef BUG_ADD_IN_GOROUTINE
	wg_add(1);
#endif
	result[me] = me + 1;
	wg_done();
	return NULL;
}

int main()
{
	pthread_t t[NWORKERS];

	for (intptr_t i = 0; i < NWORKERS; i++) {
#ifndef BUG_ADD_IN_GOROUTINE
		wg_add(1);
#endif
		if (pthread_create(&t[i], NULL, worker, (void *) i))
			abort();
	}
	wg_wait();
	for (int i = 0; i < NWORKERS; i++)
		assert(result[i] == i + 1);
	for (int i = 0; i < NWORKERS; i++)
		if (pthread_join(t[i], NULL))
			abort();
	return 0;
}
