#include <pthread.h>
#include <stdatomic.h>

/*
 * A weak CAS whose expected value two writes provide. The CAS reads one of
 * them and succeeds; its spurious failure is explored first, and the forward
 * revisit to the other write followed with the failure flag still set: the
 * CAS failed spuriously again instead of succeeding, and the execution where
 * it succeeds reading that write (in each coherence order) was lost: 8
 * executions instead of 10.
 */

atomic_int x;
int r_main;

void *t1(void *arg)
{
	atomic_store_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}

void *t2(void *arg)
{
	atomic_store_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}

int main(void)
{
	pthread_t a, b;
	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	int e = 1;
	r_main = atomic_compare_exchange_weak_explicit(&x, &e, 5, memory_order_relaxed,
						       memory_order_relaxed);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	return 0;
}
