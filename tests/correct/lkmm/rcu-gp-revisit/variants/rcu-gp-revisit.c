#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <lkmm.h>

/*
 * The litmus corpus's auto/C-RW-G+RW-R1: a grace period against a reader
 * whose write comes after its critical section. If P0 sees P1's write of x
 * and P1's reader sees P0's write of y, which comes after the grace period,
 * the critical section would both precede the grace period (P1 writes x
 * after it) and overlap its end: forbidden.
 *
 * GenMC finds the forbidden outcome by a revisit: P0's write of y revisits
 * P1's read, and keeps the events of P1 that follow it. Two bugs let that
 * graph through unchecked: rcu_read_unlock() and synchronize_rcu() were not
 * in the revisit prefix, so they were re-added later in the middle of their
 * thread, without a check; and the revisited read takes the co-maximal
 * write, for which the driver skipped the check, while RCU's rcu-link and
 * rcu-fence order by po, not by ppo.
 */

int x0;
int x1;
int r0;
int r1;

void *P0(void *unused)
{
	r0 = READ_ONCE(x0);
	synchronize_rcu();
	WRITE_ONCE(x1, 1);
	return NULL;
}

void *P1(void *unused)
{
	rcu_read_lock();
	r1 = READ_ONCE(x1);
	rcu_read_unlock();
	WRITE_ONCE(x0, 1);
	return NULL;
}

int main()
{
	pthread_t t0, t1;

	if (pthread_create(&t0, NULL, P0, NULL))
		abort();
	if (pthread_create(&t1, NULL, P1, NULL))
		abort();
	if (pthread_join(t0, NULL))
		abort();
	if (pthread_join(t1, NULL))
		abort();

	assert(!(r0 == 1 && r1 == 1));
	return 0;
}
