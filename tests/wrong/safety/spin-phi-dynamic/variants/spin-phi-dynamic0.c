#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * A loop with a header PHI (the value refreshed by a load) and a back path
 * with a store, so that the spin-assume transformation checks it
 * dynamically: a spin-start blocks the thread if the previous iteration had
 * no side effects. An iteration that reads a new value changes the PHI and
 * must not count as a repetition; the assertion fails only when the loop
 * reads 0 and then 2 (and never 1).
 */

atomic_uint x, z;

static void *thread(void *unused)
{
	int *done = malloc(sizeof(int));
	*done = 0;
	uint32_t r = atomic_load_explicit(&x, memory_order_relaxed);
	uint32_t first = r;
	for (;;) {
		if (r == 2)
			break;
		if (r == 1) {
			if (!*done) {
				atomic_store_explicit(&z, 1, memory_order_relaxed);
				*done = 1;
			}
			r = atomic_load_explicit(&x, memory_order_relaxed);
			continue;
		}
		r = atomic_load_explicit(&x, memory_order_relaxed);
	}
	uint32_t zz = atomic_load_explicit(&z, memory_order_relaxed);
	assert(!(first == 0 && zz == 0));
	free(done);
	return NULL;
}

static void *writer(void *unused)
{
	atomic_store_explicit(&x, 1, memory_order_relaxed);
	atomic_store_explicit(&x, 2, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, thread, NULL);
	pthread_create(&b, NULL, writer, NULL);
	return 0;
}
