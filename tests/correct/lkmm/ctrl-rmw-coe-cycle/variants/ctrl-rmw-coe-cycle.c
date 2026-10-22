#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <lkmm.h>

/*
 * P0 reads y from P1's release, and only then (ctrl) does a cmpxchg() on
 * x. If the cmpxchg() reads the initial x, its write goes before P1's in
 * coherence, which LKMM forbids: P0's write ->coe P1's x = 1 ->po-rel
 * P1's y = 2 ->rfe P0's read of y ->ctrl P0's write is an hb cycle (prop
 * makes its first three steps an edge in P0). GenMC placed the write of
 * the cmpxchg() after the write it read without checking the graph, and
 * explored the execution: the assertion failed (herd7: 2 executions, the
 * outcome never).
 */

int x, y;
int r0, r1 = -1;

void *P0(void *unused)
{
	r0 = READ_ONCE(y);
	if (r0)
		r1 = cmpxchg_relaxed(&x, 0, 2);
	return NULL;
}

void *P1(void *unused)
{
	WRITE_ONCE(x, 1);
	smp_store_release(&y, 2);
	return NULL;
}

int main()
{
	pthread_t t0, t1;

	if (pthread_create(&t0, NULL, P0, NULL))
		abort();
	if (pthread_create(&t1, NULL, P1, NULL))
		abort();
	if (pthread_join(t0, NULL) || pthread_join(t1, NULL))
		abort();
	assert(!(r0 == 2 && r1 == 0));
	return 0;
}
