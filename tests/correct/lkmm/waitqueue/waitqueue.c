#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <lkmm.h>

/*
 * The Linux kernel's wait-queue pattern (include/linux/wait.h), written
 * with the kernel's primitives (<lkmm.h>); tests/correct/classic/waitqueue
 * is the same model in C11 atomics. The waiter loops: prepare_to_wait()
 * adds it to the wait queue under the queue's lock and sets its state to
 * sleeping with set_current_state(), an smp_store_mb(); it then checks the
 * condition and, if false, calls schedule() to sleep until woken. The waker
 * sets the condition and wakes the queue, but to skip the lock when nobody
 * waits it first checks waitqueue_active(), a lockless read of the queue.
 * schedule() is an await on the task state.
 *
 * Without a switch the waker has the smp_mb() that wait.h requires before
 * waitqueue_active() (as in wq_has_sleeper()). WAKE_ALWAYS selects the other
 * fix: the waker calls wake_up() unconditionally, whose lock orders it.
 *
 * BUG_NO_MB: the waker checks waitqueue_active() right after setting the
 * condition, without smp_mb(). Its load of the queue can be ordered before
 * its store of the condition (store buffering with the waiter): it sees an
 * empty queue, while the waiter does not see the condition, and the waiter
 * sleeps forever (-check-liveness).
 */

#define RUNNING 0
#define SLEEPING 1

int cond;
DEFINE_SPINLOCK(wq_lock);
int nwaiters;		/* the wait list; changed under wq_lock */
int task_state;		/* the waiter's state */

static void prepare_to_wait(bool *queued)
{
	spin_lock(&wq_lock);
	if (!*queued) {
		WRITE_ONCE(nwaiters, 1);
		*queued = true;
	}
	smp_store_mb(task_state, SLEEPING);	/* set_current_state() */
	spin_unlock(&wq_lock);
}

static void finish_wait(void)
{
	WRITE_ONCE(task_state, RUNNING);
	spin_lock(&wq_lock);
	WRITE_ONCE(nwaiters, 0);
	spin_unlock(&wq_lock);
}

static void schedule(void)
{
	__VERIFIER_await(smp_load_acquire(&task_state) == RUNNING);
}

static bool waitqueue_active(void)
{
	return READ_ONCE(nwaiters) != 0;
}

static void wake_up(void)
{
	spin_lock(&wq_lock);
	if (READ_ONCE(nwaiters))
		smp_store_release(&task_state, RUNNING);
	spin_unlock(&wq_lock);
}

static void *waiter(void *unused)
{
	bool queued = false;

	for (;;) {
		prepare_to_wait(&queued);
		if (READ_ONCE(cond))
			break;
		schedule();
	}
	finish_wait();
	return NULL;
}

static void *waker(void *unused)
{
	WRITE_ONCE(cond, 1);
#if defined(WAKE_ALWAYS)
	wake_up();
#else
# ifndef BUG_NO_MB
	smp_mb();
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
