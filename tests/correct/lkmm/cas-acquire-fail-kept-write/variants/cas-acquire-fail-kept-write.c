#include <pthread.h>
#include <stdatomic.h>

/*
 * Issue #230: a C11 compare-exchange whose orderings are both acquire.
 * The model orders the store after it only when the CAS succeeds (a failed
 * CAS orders nothing, as a failed cmpxchg_acquire()). In the graph where
 * the CAS fails reading t1's store to v1, t2's store to v0 revisits t1's
 * reads and is kept without the CAS: executed anew, the CAS then read the
 * initializer and succeeded, giving graphs also reached from the graph
 * where the CAS succeeds. The optimal exploration counted 8 executions
 * instead of 6 (herd7 gives 6 with cmpxchg_acquire()).
 */

atomic_int v0, v1;
int a1, a2, b1, b2;

void *t1(void *unused)
{
	a1 = atomic_load_explicit(&v0, memory_order_relaxed);
	a2 = atomic_load_explicit(&v0, memory_order_relaxed);
	atomic_store_explicit(&v1, 1, memory_order_relaxed);
	return NULL;
}

void *t2(void *unused)
{
	b1 = atomic_load_explicit(&v0, memory_order_relaxed);
	int e = 0;
	atomic_compare_exchange_strong_explicit(&v1, &e, 1, memory_order_acquire,
						memory_order_acquire);
	b2 = e;
	atomic_store_explicit(&v0, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t a, b;

	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	return 0;
}
