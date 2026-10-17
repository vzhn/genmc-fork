#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * A failed compare_exchange writes the value it read into its expected
 * argument, so the next iteration of the retry loop compares against that
 * value (after mem2reg, a PHI in the loop header fed by the cmpxchg result).
 * The spin-assume transformation took the loop for a spinloop and pruned the
 * retry; the assertion fails only on the retry path.
 */

atomic_uint x;

static void *thread(void *unused)
{
	uint32_t old = atomic_load_explicit(&x, memory_order_relaxed);
	uint32_t first = old;
	while (!atomic_compare_exchange_strong_explicit(&x, &old, old + 1, memory_order_relaxed,
							 memory_order_relaxed))
		;
	assert(old == first); /* fails iff the CAS had to be retried */
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, thread, NULL);
	pthread_create(&b, NULL, thread, NULL);
	return 0;
}
