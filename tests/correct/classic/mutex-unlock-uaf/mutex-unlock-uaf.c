#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * A futex-based mutex in the style of glibc's lowlevellock (lock word 0:
 * free, 1: locked, 2: locked with waiters). lock() tries a CAS from 0 to 1;
 * otherwise it exchanges 2 into the word and waits on the futex until it
 * finds the word free. unlock() exchanges 0 into the word and, if it was 2,
 * wakes a waiter; the wake needs the mutex's kind (private or shared). A
 * futex wait is an await on the word.
 *
 * POSIX lets a thread destroy (and free) a mutex as soon as it has locked
 * and unlocked it, even while another thread is still returning from its
 * own unlock. So unlock() must not touch the mutex after the store that
 * releases it: it reads the kind first.
 *
 * T1 locks a heap mutex, marks its work done and unlocks; T2 locks it,
 * unlocks it and, if T1's work is done (so T1 will not lock it again),
 * frees it.
 *
 * BUG_KIND_AFTER_RELEASE: glibc bug 13690 ("pthread_mutex_unlock potentially
 * cause invalid access"): lll_unlock() evaluated its private argument,
 * PTHREAD_MUTEX_PSHARED (mutex), which reads the mutex's kind, after the
 * releasing exchange. T2 can take the mutex, unlock and free it in
 * between, and T1 reads freed memory.
 */

struct mutex {
	atomic_int lock;
	atomic_int kind;
};

struct mutex *m;
int done;		/* T1's work, guarded by m */

static void futex_wake(atomic_int *addr, int private) {}

static void mutex_lock(struct mutex *mx)
{
	int unlocked = 0;

	if (atomic_compare_exchange_strong_explicit(&mx->lock, &unlocked, 1, memory_order_acquire,
						    memory_order_relaxed))
		return;
	while (atomic_exchange_explicit(&mx->lock, 2, memory_order_acquire) != 0)
		__VERIFIER_await(atomic_load_explicit(&mx->lock, memory_order_relaxed) != 2);
}

static void mutex_unlock(struct mutex *mx)
{
#ifndef BUG_KIND_AFTER_RELEASE
	int private = atomic_load_explicit(&mx->kind, memory_order_relaxed);
#endif
	int old = atomic_exchange_explicit(&mx->lock, 0, memory_order_release);

	if (old > 1) {
#ifdef BUG_KIND_AFTER_RELEASE
		int private = atomic_load_explicit(&mx->kind, memory_order_relaxed);
#endif
		futex_wake(&mx->lock, private);
	}
}

static void *thread_1(void *unused)
{
	mutex_lock(m);
	done = 1;
	mutex_unlock(m);
	return NULL;
}

static void *thread_2(void *unused)
{
	mutex_lock(m);
	int t1_done = done;
	mutex_unlock(m);
	if (t1_done)
		free(m);	/* pthread_mutex_destroy() and free */
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	m = malloc(sizeof(*m));
	atomic_init(&m->lock, 0);
	atomic_init(&m->kind, 0);
	if (pthread_create(&t1, NULL, thread_1, NULL))
		abort();
	if (pthread_create(&t2, NULL, thread_2, NULL))
		abort();
	if (pthread_join(t1, NULL))
		abort();
	if (pthread_join(t2, NULL))
		abort();
	return 0;
}
