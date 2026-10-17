#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>
#include "../../../lib/lock.h"

/*
 * The Linux futex wait/wake handshake (kernel/futex, the ordering comment
 * above futex_wait()). Since commit b0c29f79ecea ("futex: Avoid taking the
 * hb->lock if there's nothing to wake up"), futex_wake() reads the hash
 * bucket's waiter count without the lock and returns if it is zero. The
 * waiter increments the count, issues smp_mb() (A), takes the bucket lock,
 * re-reads the futex word and queues itself if it still holds the value it
 * waits on; the waker stores the new value, issues smp_mb() (B) and checks
 * the count. The two barriers pair: either the waker sees the waiter or
 * the waiter sees the new value. Kernel primitives map to C11 (smp_mb() a
 * seq_cst fence, plain futex accesses relaxed); schedule() is an await.
 *
 * The waiter waits while the futex word is 0; the waker sets it to 1 and
 * wakes. The waiter does not sleep forever (-check-liveness).
 *
 * BUG_NO_MB: futex_wake() without barrier (B). It was meant to come from
 * get_futex_key_refs(), which issued none for private futexes, until
 * commit 76835b0ebf8a ("futex: Ensure get_futex_key_refs() always implies
 * a barrier", 2014), a hang seen even on x86. The waker's load of the
 * count can come before its store of the word: it sees no waiter while the
 * waiter sees the old value and sleeps forever. Passes under SC.
 */

atomic_int futex_word;
lock_t hb_lock;
atomic_int waiters;	/* the hash bucket's waiter count */
atomic_bool queued;	/* the waiter is on the bucket's list */
atomic_bool woken;

static void futex_wait(int val)
{
	atomic_fetch_add_explicit(&waiters, 1, memory_order_relaxed);
	atomic_thread_fence(memory_order_seq_cst);	/* (A) */
	lock_acquire(&hb_lock);
	if (atomic_load_explicit(&futex_word, memory_order_relaxed) != val) {
		lock_release(&hb_lock);
		atomic_fetch_sub_explicit(&waiters, 1, memory_order_relaxed);
		return;
	}
	atomic_store_explicit(&queued, true, memory_order_relaxed);
	lock_release(&hb_lock);
	__VERIFIER_await(atomic_load_explicit(&woken, memory_order_acquire));	/* schedule() */
	atomic_fetch_sub_explicit(&waiters, 1, memory_order_relaxed);
}

static void futex_wake(void)
{
#ifndef BUG_NO_MB
	atomic_thread_fence(memory_order_seq_cst);	/* (B) */
#endif
	if (atomic_load_explicit(&waiters, memory_order_relaxed) == 0)
		return;
	lock_acquire(&hb_lock);
	if (atomic_load_explicit(&queued, memory_order_relaxed))
		atomic_store_explicit(&woken, true, memory_order_release);
	lock_release(&hb_lock);
}

static void *waiter(void *unused)
{
	while (atomic_load_explicit(&futex_word, memory_order_relaxed) == 0)
		futex_wait(0);
	return NULL;
}

static void *waker(void *unused)
{
	atomic_store_explicit(&futex_word, 1, memory_order_relaxed);
	futex_wake();
	return NULL;
}

int main()
{
	pthread_t w, k;

	if (pthread_create(&w, NULL, waiter, NULL))
		abort();
	if (pthread_create(&k, NULL, waker, NULL))
		abort();
	if (pthread_join(w, NULL))
		abort();
	if (pthread_join(k, NULL))
		abort();
	return 0;
}
