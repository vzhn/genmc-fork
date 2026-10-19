#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>

/*
 * A branch without a join in its function (an early return): the rest of
 * the function depends on it, no LB (3 executions)
 */

int x, y;

void *P0(void *unused)
{
	int r0 = READ_ONCE(x);
	if (r0)
		return NULL;
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
