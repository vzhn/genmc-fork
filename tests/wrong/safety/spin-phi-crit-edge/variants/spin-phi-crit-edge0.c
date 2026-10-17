#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * A loop with a header PHI and a latch whose backedge is a critical edge,
 * under -check-liveness: the spin-assume transformation splits that edge to
 * mark iterations that change the PHI as new starts of the loop. The
 * assertion fails only when the loop reads 0, then 1, then 3; before the fix
 * GenMC reported a false liveness violation instead.
 */

atomic_uint x, w;

static void *thread(void *unused)
{
	uint32_t r = atomic_load_explicit(&x, memory_order_relaxed);
	uint32_t first = r;
	do {
		if (r == 1) {
			(void)atomic_load_explicit(&w, memory_order_relaxed);
			r = atomic_load_explicit(&x, memory_order_relaxed);
			if (r == 3)
				break;
		} else {
			r = atomic_load_explicit(&x, memory_order_relaxed);
		}
	} while (r != 2);
	assert(!(first == 0 && r == 3));
	return NULL;
}

static void *writer(void *unused)
{
	atomic_store_explicit(&w, 1, memory_order_relaxed);
	atomic_store_explicit(&x, 1, memory_order_relaxed);
	atomic_store_explicit(&x, 2, memory_order_relaxed);
	atomic_store_explicit(&x, 3, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, thread, NULL);
	pthread_create(&b, NULL, writer, NULL);
	return 0;
}
