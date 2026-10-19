#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>

/*
 * The branch chooses the address of the write after it: the write depends
 * on the read (an address dependency through the join), no LB (2
 * executions)
 */

int x, a, b;

void *P0(void *unused)
{
	int *p = &b;
	int r0 = READ_ONCE(x);
	if (r0)
		p = &a;
	WRITE_ONCE(*p, 1);
	return NULL;
}

void *P1(void *unused)
{
	int r1 = READ_ONCE(a);
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
