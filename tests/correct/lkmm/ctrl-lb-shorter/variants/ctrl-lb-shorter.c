#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>

/*
 * As ctrl-lb-longer, but the revisited read makes P0 skip a branch of two
 * events before the write that the revisit kept (herd7: 4 executions).
 */

int x, y, a;

void *P0(void *unused)
{
	int r0 = READ_ONCE(x);
	if (r0 == 0) {
		WRITE_ONCE(a, 1);
		WRITE_ONCE(a, 2);
	}
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
