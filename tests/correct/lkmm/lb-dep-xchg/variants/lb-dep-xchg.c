/*
 * Load buffering through a dependency into an RMW: t0 reads x and xchg()s y
 * with a data (DATA), control (CTRL) or address (ADDR) dependency on the
 * value; t1 reads y and, after smp_mb(), writes x = 1. LKMM forbids
 * r0 == 1 && r1 == 1 (herd7: Never for all three): the dependency orders the
 * RMW's write, although GenMC creates that write without dependencies of its
 * own (the kat's dependencies continue through rmw).
 *
 * Executions: DATA 3 (r0 = 0 with r1 = 0 from either write; r0 = 1 with
 * r1 = 0), CTRL 2 and ADDR 2 (t0's write only when r0 = 1, or to ys[0]).
 */
#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>
#include <genmc.h>

int x;
int ys[2];
int r0, r1;

static void *thread_0(void *unused)
{
	r0 = READ_ONCE(x);
#if defined(DATA)
	xchg_relaxed(&ys[1], r0);
#elif defined(CTRL)
	if (r0)
		xchg_relaxed(&ys[1], 1);
#elif defined(ADDR)
	xchg_relaxed(&ys[r0], 1);
#endif
	return NULL;
}

static void *thread_1(void *unused)
{
	r1 = READ_ONCE(ys[1]);
	smp_mb();
	WRITE_ONCE(x, 1);
	return NULL;
}

int main()
{
	pthread_t t0, t1;

	if (pthread_create(&t0, NULL, thread_0, NULL))
		abort();
	if (pthread_create(&t1, NULL, thread_1, NULL))
		abort();
	if (pthread_join(t0, NULL) || pthread_join(t1, NULL))
		abort();
	assert(!(r0 == 1 && r1 == 1));
	return 0;
}
