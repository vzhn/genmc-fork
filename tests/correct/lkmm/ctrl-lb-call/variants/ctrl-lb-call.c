#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>

/*
 * A call in the branch and one after it, to the same function: the
 * events of the second call keep their identity whether the first ran
 */

int x, y, a;

void touch(int v)
{
	WRITE_ONCE(a, v);
	(void) READ_ONCE(a);
}

void *P0(void *unused)
{
	int r0 = READ_ONCE(x);
	if (r0)
		touch(1);
	touch(2);
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
