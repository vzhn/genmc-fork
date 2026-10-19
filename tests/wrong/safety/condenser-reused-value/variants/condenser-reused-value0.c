#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * The code condenser merges a block into a bisimilar one that performs the
 * same operations and feeds the same values to the following PHIs. Here the
 * pre-loop load and the loop's reload are bisimilar, so the condenser routed
 * the backedge through the pre-loop block, and `first` (the pre-loop load)
 * was redefined by every iteration: the assertion became r == r.
 * Spin-assume is disabled as it handles the loop soundly on its own.
 */

atomic_int x;

static void *waiter(void *unused)
{
	int r = atomic_load_explicit(&x, memory_order_relaxed);
	int first = r;
	while (r == 0)
		r = atomic_load_explicit(&x, memory_order_relaxed);
	assert(r == first); /* fails iff the loop iterated */
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
