#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>

/*
 * A control dependency ends at the join of its branch (LKMM's ctrl): the
 * write to y after the if does not depend on the read of x, so LB is
 * allowed (herd7: 4 executions). When the read is revisited, P0 takes the
 * branch and has one event more before the write that the revisit kept.
 */

int x, y, a;

void *P0(void *unused)
{
	int r0 = READ_ONCE(x);
	if (r0)
		WRITE_ONCE(a, 1);
	WRITE_ONCE(y, 1);
	return NULL;
}

void *P1(void *unused)
{
	int r1 = READ_ONCE(y);
	WRITE_ONCE(x, r1);
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
