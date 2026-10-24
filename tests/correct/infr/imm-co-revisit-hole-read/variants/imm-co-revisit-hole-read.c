#include <pthread.h>
#include <stdatomic.h>

/*
 * Issue #228: under IMM, the execution where t0 reads t1's store, t1 t2's
 * and t2 t0's (load buffering through t2's read and write of v0, which
 * nothing orders) was missed under every policy by the default
 * exploration, and under wf and ltr by -exhaustive (14 executions of 15).
 *
 * It is built from the revisit of t0's read by t1's store, which keeps
 * t1's read of t2's store and the store, and leaves t2's read as a hole.
 * The hole is filled before t0 stores to v0, and reads the initializer.
 * The coherence revisit that places t0's store before t2's in coherence
 * then had t2's read as a candidate to revisit, but the candidates were
 * taken as the readers of the co-maximal write only, as if t0's store
 * were co-maximal.
 */

atomic_int v0, v1;
int r0, r1, r2;

void *t0(void *unused)
{
	r0 = atomic_load_explicit(&v1, memory_order_seq_cst);
	atomic_store_explicit(&v0, 1, memory_order_relaxed);
	return NULL;
}

void *t1(void *unused)
{
	r1 = atomic_load_explicit(&v0, memory_order_acquire);
	atomic_store_explicit(&v1, 1, memory_order_relaxed);
	return NULL;
}

void *t2(void *unused)
{
	r2 = atomic_load_explicit(&v0, memory_order_relaxed);
	atomic_store_explicit(&v0, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t t[3];

	pthread_create(&t[0], NULL, t0, NULL);
	pthread_create(&t[1], NULL, t1, NULL);
	pthread_create(&t[2], NULL, t2, NULL);
	return 0;
}
