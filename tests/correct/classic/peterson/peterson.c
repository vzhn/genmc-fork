#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * Two-thread locks from The Art of Multiprocessor Programming, ch. 2:
 * LockOne (Fig. 2.4), LockTwo (Fig. 2.5) and the Peterson lock (Fig. 2.6),
 * which combines them. Each thread takes the lock ROUNDS times.
 *
 * Without a switch: the Peterson lock with seq_cst accesses. No two threads
 * are in the critical section at once, and no thread waits forever.
 *
 * One switch per bug:
 * - BUG_LOCKONE: LockOne. Both threads raise their flag, then each waits for
 *   the other's flag to drop: deadlock (-check-liveness);
 * - BUG_LOCKTWO: LockTwo. A thread waits until the other one becomes the
 *   victim, which never happens once the other one is done: deadlock;
 * - BUG_RELACQ: the Peterson lock with release stores and acquire loads
 *   (ch. 7, Fig. 7.1). Correct under SC, but each thread's load of the other
 *   flag can miss the other thread's store (store buffering), so both enter:
 *   mutual exclusion fails under RC11 and IMM.
 */

#ifndef ROUNDS
#define ROUNDS 1
#endif

#ifdef BUG_RELACQ
#define ST memory_order_release
#define LD memory_order_acquire
#else
#define ST memory_order_seq_cst
#define LD memory_order_seq_cst
#endif

atomic_bool flag[2];
atomic_int victim;
atomic_int in_cs;	/* threads in the critical section */

static void lock(int me)
{
	int other = 1 - me;

#if defined(BUG_LOCKONE)
	atomic_store_explicit(&flag[me], true, ST);
	__VERIFIER_await(!atomic_load_explicit(&flag[other], LD));
#elif defined(BUG_LOCKTWO)
	atomic_store_explicit(&victim, me, ST);
	__VERIFIER_await(atomic_load_explicit(&victim, LD) != me);
#else
	atomic_store_explicit(&flag[me], true, ST);
	atomic_store_explicit(&victim, me, ST);
	__VERIFIER_await(!atomic_load_explicit(&flag[other], LD) ||
			 atomic_load_explicit(&victim, LD) != me);
#endif
}

static void unlock(int me)
{
#ifndef BUG_LOCKTWO
	atomic_store_explicit(&flag[me], false, ST);
#endif
}

static void *thread(void *arg)
{
	int me = (intptr_t) arg;

	for (int i = 0; i < ROUNDS; i++) {
		lock(me);
		int others = atomic_fetch_add_explicit(&in_cs, 1, memory_order_relaxed);
		assert(others == 0);
		atomic_fetch_sub_explicit(&in_cs, 1, memory_order_relaxed);
		unlock(me);
	}
	return NULL;
}

int main()
{
	pthread_t t[2];

	for (intptr_t i = 0; i < 2; i++)
		if (pthread_create(&t[i], NULL, thread, (void *) i))
			abort();
	for (int i = 0; i < 2; i++)
		if (pthread_join(t[i], NULL))
			abort();
	return 0;
}
