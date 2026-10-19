#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>

/* The function the read chooses writes y: no LB (3 executions) */

int x, y;

void fy(void)
{
	WRITE_ONCE(y, 1);
}

void fn(void)
{
}

void (*fns[2])(void) = {fy, fn};

void *P0(void *unused)
{
	int r0 = READ_ONCE(x);
	fns[r0 != 0]();
	return NULL;
}

void *P1(void *unused)
{
	int r1 = READ_ONCE(y);
	WRITE_ONCE(x, 1 + r1);
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
