#include <pthread.h>
#include <stdatomic.h>

/*
 * Under IMM, t2's store of x2 revisits t1's load with a prefix that keeps
 * the write part of t2's fetch_add (the acquire load of x3 depends on it)
 * but not its read part, which becomes a hole. Reconstructing the state at
 * the start of the next execution asked the write whether it is effectful,
 * which looked at the read part's operation through a null pointer
 * (segmentation fault). 2 executions.
 */

atomic_int x2, x3;
int r1, r2, r3;

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
	pthread_t a, b;
	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	return 0;
}
