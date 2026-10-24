/*
 * An assertion inside an RCU read-side critical section, on values the
 * grace-period guarantee forbids: the reader misses the write before
 * synchronize_rcu(), so its section ends before the grace period does, and
 * it cannot see the write after it. GenMC reported the violation (#201): the
 * graph was consistent, as the reader's rcu_read_unlock() was not added yet,
 * and only that unlock makes it inconsistent. An error inside an open
 * section is now checked with the section closed (I6 under
 * -check-invariants). herd7: Never, 3 executions, like
 * tests/correct/lkmm/rcu-gp2, where the assertion is after the section.
 */
#include <pthread.h>
#include <assert.h>
#include <lkmm.h>

int x = 1, y = 1;

void *updater(void *u)
{
	WRITE_ONCE(x, 0);
	synchronize_rcu();
	WRITE_ONCE(y, 0);
	return NULL;
}

void *reader(void *u)
{
	rcu_read_lock();
	int r1 = READ_ONCE(x);
	int r2 = READ_ONCE(y);
	assert(!(r1 == 1 && r2 == 0));
	rcu_read_unlock();
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, updater, NULL);
	pthread_create(&b, NULL, reader, NULL);
	return 0;
}
