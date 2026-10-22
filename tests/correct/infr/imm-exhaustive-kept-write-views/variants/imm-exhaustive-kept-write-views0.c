/* -exhaustive under -imm: t3's store revisits t0's load. The revisit keeps
 * what t3 computed from: the CAS's expected value comes from t3's load of
 * g, so that load and the store to g it reads (t2's) are kept, while t2's
 * acquire load of f before that store is not (a hole). When t2 loads f
 * again and reads t1's release store, the views of the kept store to g
 * still lack it, and so did those of the plain write after it: the write
 * raced with t1's (a false "Non-atomic race!"; the default exploration
 * finds none: x = 2 only runs after t2 has read t1's f = 1). What the
 * views leave unordered is now checked again with the views computed
 * anew.
 *
 * (Variant 1: the same, with an assertion.)
 */
#include <pthread.h>
#include <stdatomic.h>

int x;
atomic_int f;
atomic_int g;
atomic_int h;
atomic_int y;

void *t0(void *arg)
{
	(void) atomic_load_explicit(&y, memory_order_relaxed);
	return NULL;
}

void *t1(void *arg)
{
	x = 1;
	atomic_store_explicit(&f, 1, memory_order_release);
	return NULL;
}

void *t2(void *arg)
{
	int a = atomic_load_explicit(&f, memory_order_acquire);
	atomic_store_explicit(&g, 1, memory_order_relaxed);
	if (a == 1)
		x = 2;
	return NULL;
}

void *t3(void *arg)
{
	int r = atomic_load_explicit(&g, memory_order_relaxed);
	atomic_compare_exchange_strong_explicit(&h, &r, 5, memory_order_acquire,
						memory_order_acquire);
	atomic_store_explicit(&y, 1, memory_order_relaxed);
	return NULL;
}

int main(void)
{
	pthread_t h0, h1, h2, h3;
	pthread_create(&h0, NULL, t0, NULL);
	pthread_create(&h1, NULL, t1, NULL);
	pthread_create(&h2, NULL, t2, NULL);
	pthread_create(&h3, NULL, t3, NULL);
	pthread_join(h0, NULL);
	pthread_join(h1, NULL);
	pthread_join(h2, NULL);
	pthread_join(h3, NULL);
	return 0;
}
