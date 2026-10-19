#include <stdlib.h>
#include <lkmm.h>
#include <pthread.h>
#include <assert.h>

/*
 * The corpus's auto/C-LB-Lrw+R-Oc. LKMM's hb has no plain events, so the
 * plain read of u0 may read P1's write although a release follows it: herd7
 * has three executions, all racy. GenMC needs the read out of the release's
 * revisit prefix to explore the third (both reads 1); with race detection
 * off it explores all three.
 */

int u0;
int x1;

void *P0(void *unused)
{
	int r1 = u0;
	smp_store_release(&x1, 1);
	return NULL;
}

void *P1(void *unused)
{
	int r1 = READ_ONCE(x1);
	if (r1 == 1)
		u0 = 1;
	return NULL;
}

int main()
{
	pthread_t t0, t1;

	pthread_create(&t0, NULL, P0, NULL);
	pthread_create(&t1, NULL, P1, NULL);

	return 0;
}
