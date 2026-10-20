#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>

/*
 * auto/C-LB-LRW+R-Oc+Oq-A (herd7: 8 executions). P1's write to x2 after the
 * if comes after the branch's write to x2 (moi): it depends on P1's read
 * when the branch is taken, and must then always, or GenMC loses the
 * execution where P2 reads it, P1 skips the branch and P0 reads P2's write.
 */

int u0, x1, x2;

void *P0(void *unused)
{
	(void) READ_ONCE(u0);
	smp_store_release(&x1, 1);
	return NULL;
}

void *P1(void *unused)
{
	if (READ_ONCE(x1) == 1)
		WRITE_ONCE(x2, 2);
	WRITE_ONCE(x2, 1);
	return NULL;
}

void *P2(void *unused)
{
	(void) smp_load_acquire(&x2);
	WRITE_ONCE(u0, 1);
	return NULL;
}

int main()
{
	pthread_t t0, t1, t2;

	if (pthread_create(&t0, NULL, P0, NULL))
		abort();
	if (pthread_create(&t1, NULL, P1, NULL))
		abort();
	if (pthread_create(&t2, NULL, P2, NULL))
		abort();
	return 0;
}
