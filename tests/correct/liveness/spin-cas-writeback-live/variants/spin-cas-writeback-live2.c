#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * As spin-cas-writeback-live0, but the expected value is refreshed by a load
 * at the end of the iteration instead of the CAS's write-back: the header PHI
 * is fed by that load and still changes when the CAS fails.
 */

atomic_uint x;

static void *thread(void *unused)
{
	uint32_t old = atomic_load_explicit(&x, memory_order_relaxed);
	for (;;) {
		uint32_t expected = old;
		if (atomic_compare_exchange_strong_explicit(&x, &expected, old + 1,
							    memory_order_relaxed,
							    memory_order_relaxed))
			break;
		old = atomic_load_explicit(&x, memory_order_relaxed);
	}
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, thread, NULL);
	pthread_create(&b, NULL, thread, NULL);
	return 0;
}
