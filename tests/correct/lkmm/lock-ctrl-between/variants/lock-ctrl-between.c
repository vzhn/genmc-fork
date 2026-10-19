/*
 * A thread takes a lock under a condition it read, between two critical
 * sections of another thread. Four executions: the reader sees nwaiters
 * initial (no lock), the second critical section's 0 (no lock), or the
 * first's 1 and then locks between the two critical sections or after both.
 *
 * Regression test: GenMC makes the write of an RMW from its read, without
 * dependencies of its own, so the kernel model's dependencies into an RMW
 * (ctrl ; [W] here, READ_ONCE(nwaiters) -> spin_lock()) must reach the
 * write through rmw. Without that the lock's write is not in the prefix of
 * the read it depends on, the revisit of the other thread's second lock is
 * rejected as non-maximal, and the "between" execution is lost (3 instead
 * of 4 under -lkmm).
 *
 * ADDR: the waker takes locks[nwaiters] (an address dependency); the
 * waiter uses locks[1]. Four executions as above.
 * DATA: the waker xchg()s tok with the value it read (a data dependency),
 * and the waiter reads tok in its second critical section: three values for
 * the waker's read times two for the waiter's: six executions.
 * Both also pass without the fix (the lost revisit needs the control
 * dependency); they pin the semantics of the other two dependencies.
 */
#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>

DEFINE_SPINLOCK(lock);
spinlock_t locks[2] = {__SPIN_LOCK_UNLOCKED(locks[0]), __SPIN_LOCK_UNLOCKED(locks[1])};
int nwaiters;
int tok;

#ifdef ADDR
#define WLOCK (&locks[1])
#else
#define WLOCK (&lock)
#endif

static void *waiter(void *unused)
{
	spin_lock(WLOCK);
	WRITE_ONCE(nwaiters, 1);
	spin_unlock(WLOCK);
	spin_lock(WLOCK);
	WRITE_ONCE(nwaiters, 0);
#ifdef DATA
	READ_ONCE(tok);
#endif
	spin_unlock(WLOCK);
	return NULL;
}

static void *waker(void *unused)
{
#if defined(ADDR)
	int i = READ_ONCE(nwaiters);

	spin_lock(&locks[i]);
	spin_unlock(&locks[i]);
#elif defined(DATA)
	xchg_relaxed(&tok, READ_ONCE(nwaiters));
#else
	if (READ_ONCE(nwaiters)) {
		spin_lock(&lock);
		spin_unlock(&lock);
	}
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
	return 0;
}
