#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <lkmm.h>

/*
 * rcu-sync-free with SRCU: P1 replaces x by z in an SRCU-protected pointer
 * and asks for x to be freed (a release store to c); P2 notices the request
 * (an acquire load), waits for a grace period of the srcu_struct and frees
 * x, as call_srcu()'s callback would. A reader that found x reads its value
 * 1, as the grace period orders its critical section before the free.
 *
 * BUG_NO_GP: the callback frees x without waiting for a grace period.
 * BUG_OTHER_SRCU: the grace period is of another srcu_struct, which does
 * not wait for the reader. Either way the reader can use x after it is
 * freed.
 */

struct srcu_struct s;
#ifdef BUG_OTHER_SRCU
struct srcu_struct other;
#define GP_SRCU other
#else
#define GP_SRCU s
#endif
int z = 1;
int *y;
int c;
int r1;

void *P0(void *unused)
{
	int idx = srcu_read_lock(&s);
	int *r0 = srcu_dereference(y, &s);
	r1 = READ_ONCE(*r0);
	srcu_read_unlock(&s, idx);
	return NULL;
}

void *P1(void *unused)
{
	rcu_assign_pointer(y, &z);
	smp_store_release(&c, 1); /* call_srcu(x) */
	return NULL;
}

void *P2(void *x)
{
	if (smp_load_acquire(&c)) {
#ifndef BUG_NO_GP
		synchronize_srcu(&GP_SRCU);
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
