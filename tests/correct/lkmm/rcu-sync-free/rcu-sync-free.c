#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <lkmm.h>

/*
 * The kernel's Documentation/litmus-tests/rcu/RCU+sync+free, with call_rcu()
 * emulated as tools/memory-model/Documentation/litmus-tests.txt does:
 * P1 replaces x by z in an RCU-protected pointer and asks for x to be freed
 * (a release store to c); P2 notices the request (an acquire load), waits
 * for a grace period and frees x, as the RCU callback would. A reader that
 * found x reads its value 1, as the grace period orders its critical
 * section before the free.
 *
 * BUG_NO_GP: the callback frees x without waiting for a grace period; the
 * reader can use x after it is freed.
 */

int z = 1;
int *y;
int c;
int r1;

void *P0(void *unused)
{
	rcu_read_lock();
	int *r0 = rcu_dereference(y);
	r1 = READ_ONCE(*r0);
	rcu_read_unlock();
	return NULL;
}

void *P1(void *unused)
{
	rcu_assign_pointer(y, &z);
	smp_store_release(&c, 1); /* call_rcu(x) */
	return NULL;
}

void *P2(void *x)
{
	if (smp_load_acquire(&c)) {
#ifndef BUG_NO_GP
		synchronize_rcu();
#endif
		free(x); /* the callback */
	}
	return NULL;
}

int main()
{
	pthread_t t0, t1, t2;
	int *x = malloc(sizeof(*x));

	*x = 1;
	y = x;
	if (pthread_create(&t0, NULL, P0, NULL))
		abort();
	if (pthread_create(&t1, NULL, P1, NULL))
		abort();
	if (pthread_create(&t2, NULL, P2, x))
		abort();
	if (pthread_join(t0, NULL))
		abort();
	if (pthread_join(t1, NULL))
		abort();
	if (pthread_join(t2, NULL))
		abort();
	assert(r1 == 1);
	return 0;
}
