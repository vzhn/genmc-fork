#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>

/* A loop in the branch: the write after it does not depend on the read */

int x, y, a;

void *P0(void *unused)
{
	int r0 = READ_ONCE(x);
	if (r0)
		for (int i = 0; i < 3; i++)
			WRITE_ONCE(a, i);
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
