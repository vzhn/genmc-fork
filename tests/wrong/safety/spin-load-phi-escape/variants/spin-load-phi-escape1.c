#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * As spin-load-phi-escape0, with a float-typed loop value and another read in
 * the loop body.
 */

_Atomic float x;
atomic_int w;

static void *waiter(void *unused)
{
	float r = atomic_load_explicit(&x, memory_order_relaxed);
	float first = r;
	while (r == 0.0f) {
		(void)atomic_load_explicit(&w, memory_order_relaxed);
		r = atomic_load_explicit(&x, memory_order_relaxed);
	}
	assert(!(first == 0.0f && r == 1.0f)); /* reachable: 0, then 1 */
	return NULL;
}

static void *writer(void *unused)
{
	atomic_store_explicit(&x, 1.0f, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, waiter, NULL);
	pthread_create(&b, NULL, writer, NULL);
	return 0;
}
