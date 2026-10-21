/* Issue #128 (scripts/fuzz-c11.py seed 915): under IMM, 4 executions were
 * explored twice (64 instead of 60; 46 under rc11, 28 under sc). t1's
 * W(x0, 1) revisits t0's fetch_add from two graphs: one where the fetch_add
 * reads t1's W(x0, 2) (W(x0, 2) revisited it earlier), and one where it
 * reads t0's own W(x0, 1), with W(x0, 2) kept and co-after it. Only the
 * first is a maximal extension, but isCoBeforeSavedPrefix() did not count
 * W(x0, 2) as an alternative for the fetch_add in the second, because the
 * prefix of W(x0, 2) reaches t0's W(x1, 0), past the fetch_add, through a
 * hole: it revisited the fetch_add all the same in the first graph.
 *
 * IMM orders the fetch_add's read (an exclusive read) before t0's later
 * events, W(x1, 0) included ([Rex] ; po), which GenMC's IMM did not: 14 of
 * the 60 executions have the fetch_add read one of t1's stores to x0
 * while t1's load or CAS of x1 reads W(x1, 0), a cycle in ar. Under IMM,
 * as herd7 with IMM's model, there are 46, as under rc11; the fetch_add is
 * in the prefix of W(x0, 2), and the path above is no longer reached. */
#include <pthread.h>
#include <stdatomic.h>

atomic_int x0;
atomic_int x1;
atomic_int x2;

void *t0(void *arg)
{
	int r0 = 0;
	int r1 = 0;
	int r2 = 0;
	int r3 = 0;
	atomic_store_explicit(&x0, 1, memory_order_release);
	r0 = atomic_fetch_add_explicit(&x0, 1, memory_order_release);
	atomic_store_explicit(&x1, 0, memory_order_relaxed);
	r2 = 2;
	r1 = atomic_compare_exchange_strong_explicit(&x0, &r2, 3, memory_order_release, memory_order_relaxed);
	r3 = atomic_load_explicit(&x2, memory_order_relaxed);
	return NULL;
}

void *t1(void *arg)
{
	int r0 = 0;
	int r1 = 0;
	int r2 = 0;
	int r3 = 0;
	r0 = atomic_load_explicit(&x1, memory_order_relaxed);
	r2 = 1;
	r1 = atomic_compare_exchange_weak_explicit(&x1, &r2, 1, memory_order_seq_cst, memory_order_relaxed);
	r3 = atomic_load_explicit(&x0, memory_order_acquire);
	atomic_store_explicit(&x0, 2, memory_order_release);
	atomic_store_explicit(&x0, 1, memory_order_seq_cst);
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
