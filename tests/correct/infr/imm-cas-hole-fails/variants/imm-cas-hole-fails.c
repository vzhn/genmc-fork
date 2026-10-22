/* Issue #137: under IMM, one execution was lost (3 instead of 4, as under
 * rc11): both CASes fail, t0's reading t1's x0 = 1 and t1's reading the
 * initial x1.
 *
 * Since #133 t1's x0 = 1 did not depend on t1's CAS (the store of the
 * CAS's expected value is dead), so the backward revisit of t0's CAS by
 * x0 = 1 kept the store and deleted the CAS before it, which became a hole.
 * Re-executed reading the initial value, the CAS fails and has no write
 * part, which changes where t1's kept store is: that execution was not
 * produced. IMM orders a CAS's read before all later events of its thread
 * ([Rex] ; po), so the CAS is in the prefix of the store and is kept with
 * its outcome. */
#include <pthread.h>
#include <stdatomic.h>

atomic_int x0;
atomic_int x1;

void *t0(void *arg)
{
	int e = 0;
	atomic_store_explicit(&x1, 3, memory_order_release);
	atomic_compare_exchange_strong_explicit(&x0, &e, 2, memory_order_acq_rel, memory_order_acquire);
	return NULL;
}

void *t1(void *arg)
{
	int e = 3;
	atomic_compare_exchange_strong_explicit(&x1, &e, 0, memory_order_seq_cst, memory_order_relaxed);
	atomic_store_explicit(&x0, 1, memory_order_relaxed);
	return NULL;
}

int main(void)
{
	pthread_t h0, h1;
	pthread_create(&h0, NULL, t0, NULL);
	pthread_create(&h1, NULL, t1, NULL);
	pthread_join(h0, NULL);
	pthread_join(h1, NULL);
	return 0;
}
