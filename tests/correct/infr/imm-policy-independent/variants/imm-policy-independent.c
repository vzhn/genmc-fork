#include <pthread.h>
#include <stdatomic.h>

/*
 * Issue #228: under IMM, the number of executions depended on the
 * scheduling policy and seed, in the default exploration and under
 * -exhaustive (126 under wf, up to 144 under arbitrary). 144 under every
 * policy since the revisits of a write moved in coherence are complete
 * (see imm-co-revisit-hole-read).
 */

atomic_int v0, v1;
int a1, a2, b1, b2, c1, d1, d2;
void *t1(void *a)
{
	a1 = atomic_load_explicit(&v0, memory_order_relaxed);
	a2 = atomic_load_explicit(&v1, memory_order_seq_cst);
	atomic_store_explicit(&v0, 1, memory_order_relaxed);
	return NULL;
}
void *t2(void *a)
{
	b1 = atomic_load_explicit(&v0, memory_order_relaxed);
	b2 = atomic_load_explicit(&v0, memory_order_acquire);
	atomic_store_explicit(&v1, 1, memory_order_relaxed);
	return NULL;
}
void *t3(void *a)
{
	c1 = atomic_load_explicit(&v0, memory_order_relaxed);
	return NULL;
}
void *t4(void *a)
{
	d1 = atomic_load_explicit(&v0, memory_order_relaxed);
	d2 = atomic_load_explicit(&v0, memory_order_relaxed);
	atomic_store_explicit(&v0, 1, memory_order_relaxed);
	return NULL;
}
int main()
{
	pthread_t a, b, c, d;
	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	pthread_create(&c, NULL, t3, NULL);
	pthread_create(&d, NULL, t4, NULL);
	return 0;
}
