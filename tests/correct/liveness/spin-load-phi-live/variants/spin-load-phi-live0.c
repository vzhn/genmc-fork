#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * A loop that refreshes a loaded value at the end of each iteration, with the
 * initial value also used after the loop: the value stays in a header PHI. An
 * iteration that reads a new value changes the PHI, so the loop is not stuck
 * when that read returns the latest value.
 */

atomic_uint x;
atomic_uint seen;

static void *waiter(void *unused)
{
	uint32_t r = atomic_load_explicit(&x, memory_order_relaxed);
	uint32_t first = r;
	while (r == 0)
		r = atomic_load_explicit(&x, memory_order_relaxed);
	atomic_store_explicit(&seen, r - first, memory_order_relaxed);
	return NULL;
}

static void *writer(void *unused)
{
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
