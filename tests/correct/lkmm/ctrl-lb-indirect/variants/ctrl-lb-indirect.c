#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>

/*
 * The read chooses the function called: its events depend on the read,
 * the write after the call does not (herd7 has no calls; 4 executions as
 * for an if)
 */

int x, y, a, b;

void fa(void)
{
	WRITE_ONCE(a, 1);
	WRITE_ONCE(a, 2);
}

void fb(void)
{
	WRITE_ONCE(b, 1);
}

void (*fns[2])(void) = {fb, fa};

void *P0(void *unused)
{
	int r0 = READ_ONCE(x);
	fns[r0 != 0]();
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
