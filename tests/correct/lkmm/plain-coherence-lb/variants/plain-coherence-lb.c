#include <stdlib.h>
#include <lkmm.h>
#include <pthread.h>
#include <assert.h>

/*
 * Load buffering through a plain read: P0's read of x is ordered before
 * its write of y (smp_mb()), and P1 writes x only after an acquire of y.
 * LKMM's hb has no plain events, so no hb cycle forbids the read from
 * seeing P1's write; plain-coherence does (wr-incoh: the read is before
 * the write by rw-xbstar). There is no race either way.
 */

int x;
int y;

void *P0(void *unused)
{
	int r0 = x;
	smp_mb();
	WRITE_ONCE(y, 1);
	assert(r0 == 0);
	return NULL;
}

void *P1(void *unused)
{
	if (smp_load_acquire(&y))
		x = 1;
	return NULL;
}

int main()
{
	pthread_t t0, t1;

	pthread_create(&t0, NULL, P0, NULL);
	pthread_create(&t1, NULL, P1, NULL);

	return 0;
}
