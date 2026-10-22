/* -exhaustive under -imm: as variant 0, with an atomic d for x and an
 * assertion for the second write. The views of t2's kept store to g lack
 * t1's release store, which t2 reads again, and so did those of the load
 * after it: the load could read the initial d, which t1's d = 1 overwrote
 * before the release store t2 read, and the assertion failed (a false
 * "Safety violation!"; under IMM, the load reads 1). An error is now
 * reported only if the graph is consistent with its views computed
 * anew. */
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_int d;
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
	atomic_store_explicit(&d, 1, memory_order_relaxed);
	atomic_store_explicit(&f, 1, memory_order_release);
	return NULL;
}

void *t2(void *arg)
{
	int a = atomic_load_explicit(&f, memory_order_acquire);
	atomic_store_explicit(&g, 1, memory_order_relaxed);
	if (a == 1)
		assert(atomic_load_explicit(&d, memory_order_relaxed) == 1);
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
