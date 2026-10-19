#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>
#include "../../../lib/lock.h"

/*
 * The Linux kernel's wait-queue pattern (include/linux/wait.h). The waiter
 * loops: prepare_to_wait() adds it to the wait queue under the queue's lock
 * and sets its state to sleeping with set_current_state(), which is a store
 * and smp_mb(); it then checks the condition and, if false, calls
 * schedule() to sleep until woken. The waker sets the condition and wakes
 * the queue, but to skip the lock when nobody waits it first checks
 * waitqueue_active(), a lockless read of the queue. Kernel primitives map
 * to C11: smp_mb() is a seq_cst fence, READ_ONCE/WRITE_ONCE relaxed
 * accesses, and the wait-queue spinlock a lock_t. schedule() is an await on
 * the task state.
 *
 * Without a switch the waker has the smp_mb() that wait.h requires before
 * waitqueue_active() (as in wq_has_sleeper()). WAKE_ALWAYS selects the other
 * fix: the waker calls wake_up() unconditionally, whose lock orders it.
 *
 * BUG_NO_MB: the waker checks waitqueue_active() right after setting the
 * condition, without smp_mb(). Its load of the queue can be ordered before
 * its store of the condition (store buffering with the waiter): it sees an
 * empty queue, while the waiter does not see the condition, and the waiter
 * sleeps forever (-check-liveness). Kosuke Tatsukawa fixed this in many
 * drivers and subsystems in 2015 ("fix waitqueue_active without memory
 * barrier in ..."). Passes under SC.
 */

#define RUNNING 0
#define SLEEPING 1

atomic_int cond;
lock_t wq_lock;
atomic_int nwaiters;		/* the wait list; changed under wq_lock */
atomic_int task_state;		/* the waiter's state */

static void prepare_to_wait(bool *queued)
{
	lock_acquire(&wq_lock);
	if (!*queued) {
		atomic_store_explicit(&nwaiters, 1, memory_order_relaxed);
		*queued = true;
	}
	/* set_current_state(): smp_store_mb() */
	atomic_store_explicit(&task_state, SLEEPING, memory_order_relaxed);
	atomic_thread_fence(memory_order_seq_cst);
	lock_release(&wq_lock);
}

static void finish_wait(void)
{
	atomic_store_explicit(&task_state, RUNNING, memory_order_relaxed);
	lock_acquire(&wq_lock);
	atomic_store_explicit(&nwaiters, 0, memory_order_relaxed);
	lock_release(&wq_lock);
}

static void schedule(void)
{
	__VERIFIER_await(atomic_load_explicit(&task_state, memory_order_acquire) == RUNNING);
}

static bool waitqueue_active(void)
{
	return atomic_load_explicit(&nwaiters, memory_order_relaxed) != 0;
}

static void wake_up(void)
{
	lock_acquire(&wq_lock);
	if (atomic_load_explicit(&nwaiters, memory_order_relaxed))
		atomic_store_explicit(&task_state, RUNNING, memory_order_release);
	lock_release(&wq_lock);
}

static void *waiter(void *unused)
{
	bool queued = false;

	for (;;) {
		prepare_to_wait(&queued);
		if (atomic_load_explicit(&cond, memory_order_relaxed))
			break;
		schedule();
	}
	finish_wait();
	return NULL;
}

static void *waker(void *unused)
{
	atomic_store_explicit(&cond, 1, memory_order_relaxed);
#if defined(WAKE_ALWAYS)
	wake_up();
#else
# ifndef BUG_NO_MB
	atomic_thread_fence(memory_order_seq_cst);
# endif
	if (waitqueue_active())
		wake_up();
#endif
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
