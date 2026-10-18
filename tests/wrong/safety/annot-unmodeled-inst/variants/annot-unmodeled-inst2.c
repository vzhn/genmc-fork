#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * As annot-unmodeled-inst0, with the unmodeled comparison on a value loaded
 * before the loop and combined with the spin condition.
 */

atomic_int x;
_Atomic float g;

static void *waiter(void *unused)
{
	float f = atomic_load_explicit(&g, memory_order_relaxed);
	while (atomic_load_explicit(&x, memory_order_relaxed) == 0 || f == 0.0f)
		;
	assert(0); /* reachable: g = 1.0, x = 1 */
	return NULL;
}

static void *writer(void *unused)
{
	atomic_store_explicit(&g, 1.0f, memory_order_relaxed);
	atomic_store_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, waiter, NULL);
	pthread_create(&b, NULL, writer, NULL);
	return 0;
}
