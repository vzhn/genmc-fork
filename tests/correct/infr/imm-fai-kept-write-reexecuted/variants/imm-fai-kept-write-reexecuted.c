#include <pthread.h>
#include <stdatomic.h>

/*
 * Under IMM, t2's store of x2 revisits t1's load with a prefix that keeps
 * the write part of t2's fetch_add but not its read part, which is executed
 * anew and may now read t0's write. The kept write part was left as it was
 * (its old value and coherence position: an inconsistent execution was
 * counted), and when the read's rf was changed by a revisit, the write part
 * was taken for an unrelated kept event and pushed one position down
 * (segmentation fault, a store where a load was due). 6 executions.
 */

atomic_int x2, x3;
int r1, r2, r3;

void *t0(void *arg)
{
	atomic_store_explicit(&x3, 2, memory_order_relaxed);
	return NULL;
}

void *t1(void *arg)
{
	r1 = atomic_load_explicit(&x2, memory_order_acquire);
	return NULL;
}

void *t2(void *arg)
{
	r2 = atomic_fetch_add_explicit(&x3, 2, memory_order_relaxed);
	r3 = atomic_load_explicit(&x3, memory_order_acquire);
	atomic_store_explicit(&x2, 2, memory_order_relaxed);
	return NULL;
}

int main(void)
{
	pthread_t a, b, c;
	pthread_create(&a, NULL, t0, NULL);
	pthread_create(&b, NULL, t1, NULL);
	pthread_create(&c, NULL, t2, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	pthread_join(c, NULL);
	return 0;
}
