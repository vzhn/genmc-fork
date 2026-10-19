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
 */
#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>

DEFINE_SPINLOCK(lock);
int nwaiters;

static void *waiter(void *unused)
{
	spin_lock(&lock);
	WRITE_ONCE(nwaiters, 1);
	spin_unlock(&lock);
	spin_lock(&lock);
	WRITE_ONCE(nwaiters, 0);
	spin_unlock(&lock);
	return NULL;
}

static void *waker(void *unused)
{
	if (READ_ONCE(nwaiters)) {
		spin_lock(&lock);
		spin_unlock(&lock);
	}
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
