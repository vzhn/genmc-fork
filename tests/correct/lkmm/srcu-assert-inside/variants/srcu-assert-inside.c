/*
 * tests/correct/lkmm/rcu-assert-inside with SRCU: an assertion inside an
 * SRCU read-side critical section, on values the grace period of its
 * srcu_struct forbids (#201). herd7: Never, 3 executions.
 */
#include <pthread.h>
#include <assert.h>
#include <lkmm.h>

struct srcu_struct s;
int x = 1, y = 1;

void *updater(void *u)
{
	WRITE_ONCE(x, 0);
	synchronize_srcu(&s);
	WRITE_ONCE(y, 0);
	return NULL;
}

void *reader(void *u)
{
	int idx = srcu_read_lock(&s);
	int r1 = READ_ONCE(x);
	int r2 = READ_ONCE(y);
	assert(!(r1 == 1 && r2 == 0));
	srcu_read_unlock(&s, idx);
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, updater, NULL);
	pthread_create(&b, NULL, reader, NULL);
	return 0;
}
