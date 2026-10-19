#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>

/*
 * A grace period of one srcu_struct does not wait for the readers of
 * another (the kernel model's & loc on SRCU's rcu-order): P1 may see P0's
 * second write and not its first. herd7: Sometimes, 4 executions.
 */

struct srcu_struct s;
struct srcu_struct t;
int x;
int y;

void *P0(void *unused)
{
	int idx = srcu_read_lock(&s);
	WRITE_ONCE(x, 1);
	WRITE_ONCE(y, 1);
	srcu_read_unlock(&s, idx);
	return NULL;
}

void *P1(void *unused)
{
	int r1 = READ_ONCE(y);
	synchronize_srcu(&t);
	int r2 = READ_ONCE(x);
	(void) r1;
	(void) r2;
	return NULL;
}

int main()
{
	pthread_t t0, t1;

	if (pthread_create(&t0, NULL, P0, NULL))
		abort();
	if (pthread_create(&t1, NULL, P1, NULL))
		abort();
	return 0;
}
