#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>

/*
 * Misuses of RCU's read-side critical sections that the kernel's lockdep
 * reports; GenMC reports them as invalid RCU usage under -lkmm. (LKMM
 * leaves unmatched rcu_read_lock()s and rcu_read_unlock()s out of its
 * critical sections and flags them; a synchronize_rcu() inside a critical
 * section makes every execution inconsistent, so none would be seen.)
 *
 * UNLOCK_WITHOUT_LOCK: rcu_read_unlock() with no critical section open.
 * SYNC_IN_SECTION: synchronize_rcu() inside a critical section, which
 * waits for its own reader.
 * ENDS_IN_SECTION: a thread returns inside a critical section.
 */

int x;

void *thread(void *unused)
{
#ifdef UNLOCK_WITHOUT_LOCK
	rcu_read_lock();
	WRITE_ONCE(x, 1);
	rcu_read_unlock();
	rcu_read_unlock();
#endif
#ifdef SYNC_IN_SECTION
	rcu_read_lock();
	WRITE_ONCE(x, 1);
	synchronize_rcu();
	rcu_read_unlock();
#endif
#ifdef ENDS_IN_SECTION
	rcu_read_lock();
	WRITE_ONCE(x, 1);
#endif
	return NULL;
}

int main()
{
	pthread_t t;

	if (pthread_create(&t, NULL, thread, NULL))
		abort();
	if (pthread_join(t, NULL))
		abort();
	return 0;
}
