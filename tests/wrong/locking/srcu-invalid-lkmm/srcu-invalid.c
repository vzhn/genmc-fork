#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>

/*
 * Misuses of SRCU's read-side critical sections that the kernel model
 * flags (linux-kernel.bell); GenMC reports them as invalid RCU usage under
 * -lkmm. An srcu_read_unlock() matches the srcu_read_lock() of the same
 * srcu_struct whose value it writes back (srcu-rscs).
 *
 * UNMATCHED_LOCK: an srcu_read_lock() that no unlock matches.
 * UNMATCHED_UNLOCK: an srcu_read_unlock() of an index no lock returned.
 * MULTIPLE_MATCHES: two unlocks of the index of one lock.
 * BAD_VALUE: an unlock of a value derived from, but other than, its lock's.
 * OTHER_STRUCT: an unlock of another srcu_struct than its lock's.
 * SYNC_IN_RCU_SECTION: synchronize_srcu() in an RCU read-side critical
 * section (invalid-sleep).
 * SYNC_IN_OWN_SECTION: synchronize_srcu() in a critical section of its
 * srcu_struct, which waits for its own reader (the model has no
 * consistent execution, and herd7 no flag).
 */

struct srcu_struct s1;
struct srcu_struct s2;
int x;

void *thread(void *unused)
{
	int idx;

#ifdef UNMATCHED_LOCK
	idx = srcu_read_lock(&s1);
	WRITE_ONCE(x, idx);
#endif
#ifdef UNMATCHED_UNLOCK
	WRITE_ONCE(x, 1);
	srcu_read_unlock(&s1, 0);
#endif
#ifdef MULTIPLE_MATCHES
	idx = srcu_read_lock(&s1);
	WRITE_ONCE(x, 1);
	srcu_read_unlock(&s1, idx);
	srcu_read_unlock(&s1, idx);
#endif
#ifdef BAD_VALUE
	idx = srcu_read_lock(&s1);
	WRITE_ONCE(x, 1);
	srcu_read_unlock(&s1, idx + 1);
#endif
#ifdef OTHER_STRUCT
	idx = srcu_read_lock(&s1);
	WRITE_ONCE(x, 1);
	srcu_read_unlock(&s2, idx);
#endif
#ifdef SYNC_IN_OWN_SECTION
	idx = srcu_read_lock(&s1);
	WRITE_ONCE(x, 1);
	synchronize_srcu(&s1);
	srcu_read_unlock(&s1, idx);
#endif
#ifdef SYNC_IN_RCU_SECTION
	rcu_read_lock();
	WRITE_ONCE(x, 1);
	synchronize_srcu(&s1);
	rcu_read_unlock();
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
