#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <lkmm.h>

/*
 * The Linux futex wait/wake handshake (kernel/futex, the ordering comment
 * above futex_wait()), written with the kernel's primitives (<lkmm.h>);
 * tests/correct/classic/futex-wake is the same model in C11 atomics. Since
 * commit b0c29f79ecea ("futex: Avoid taking the hb->lock if there's nothing
 * to wake up"), futex_wake() reads the hash bucket's waiter count without
 * the lock and returns if it is zero. The waiter increments the count,
 * issues smp_mb() (A), takes the bucket lock, re-reads the futex word and
 * queues itself if it still holds the value it waits on; the waker stores
 * the new value, issues smp_mb() (B) and checks the count. The two barriers
 * pair: either the waker sees the waiter or the waiter sees the new value.
 * schedule() is an await.
 *
 * The waiter waits while the futex word is 0; the waker sets it to 1 and
 * wakes. The waiter does not sleep forever (-check-liveness).
 *
 * BUG_NO_MB: futex_wake() without barrier (B). It was meant to come from
 * get_futex_key_refs(), which issued none for private futexes, until
 * commit 76835b0ebf8a ("futex: Ensure get_futex_key_refs() always implies
 * a barrier", 2014), a hang seen even on x86. The waker's load of the
 * count can come before its store of the word: it sees no waiter while the
 * waiter sees the old value and sleeps forever.
 */

int futex_word;
DEFINE_SPINLOCK(hb_lock);
atomic_t waiters = ATOMIC_INIT(0);	/* the hash bucket's waiter count */
int queued;				/* the waiter is on the bucket's list */
int woken;

static void futex_wait(int val)
{
	atomic_inc(&waiters);			/* hb_waiters_inc() */
	smp_mb();				/* (A) */
	spin_lock(&hb_lock);
	if (READ_ONCE(futex_word) != val) {
		spin_unlock(&hb_lock);
		atomic_dec(&waiters);
		return;
	}
	WRITE_ONCE(queued, 1);
	spin_unlock(&hb_lock);
	__VERIFIER_await(smp_load_acquire(&woken));	/* schedule() */
	atomic_dec(&waiters);
}

static void futex_wake(void)
{
#ifndef BUG_NO_MB
	smp_mb();				/* (B) */
#endif
	if (!atomic_read(&waiters))		/* hb_waiters_pending() */
		return;
	spin_lock(&hb_lock);
	if (READ_ONCE(queued))
		smp_store_release(&woken, 1);
	spin_unlock(&hb_lock);
}

static void *waiter(void *unused)
{
	while (READ_ONCE(futex_word) == 0)
		futex_wait(0);
	return NULL;
}

static void *waker(void *unused)
{
	WRITE_ONCE(futex_word, 1);
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
