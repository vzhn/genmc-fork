/*
 * LB through the expected value of a cmpxchg (herd7: Sometimes; 3 states).
 * The expected value orders nothing in LKMM: P0's cmpxchg may succeed with
 * r0 = 1 while P1 reads its 2. GenMC records the compare operand's
 * dependencies (EventDeps::cas) but no checker reads them, and the revisit
 * that builds the cycle is never consistent: GenMC misses r0 = 1 && r2 = 2
 * (2 executions) under -lkmm and -imm. Known gap; in cooking until fixed.
 */
#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>

int x;
int y = 1;
int r0, r1, r2;

static void *P0(void *unused)
{
	r0 = READ_ONCE(x);
	r1 = cmpxchg_relaxed(&y, r0, 2);
	return NULL;
}

static void *P1(void *unused)
{
	r2 = READ_ONCE(y);
	smp_mb();
	WRITE_ONCE(x, 1);
	return NULL;
}

int main()
{
	pthread_t p0, p1;

	if (pthread_create(&p0, NULL, P0, NULL))
		abort();
	if (pthread_create(&p1, NULL, P1, NULL))
		abort();
	return 0;
}
