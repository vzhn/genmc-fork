#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <assert.h>
#include <genmc.h>

/*
 * The condenser merged two blocks from their first bisimilar point, even
 * when the surviving block's suffix used a value defined earlier in that
 * block (which the redirected paths never define). Here the comparisons
 * and branches of the pre-loop block and the loop body are bisimilar, but
 * the stores before them are not (a != b), so the loop was turned into a
 * loop over the pre-loop comparison alone, and its body (the store of 0 to
 * y) was never executed.
 */

atomic_int x;
atomic_int y;

static void *waiter(void *arg)
{
	int a = (int)(intptr_t)arg;
	int b = a + 1;
	int r = atomic_load_explicit(&x, memory_order_relaxed);
	atomic_store_explicit(&y, r ^ a, memory_order_relaxed);
	int c = (r == 0);
	while (c) {
		r = atomic_load_explicit(&x, memory_order_relaxed);
		atomic_store_explicit(&y, r ^ b, memory_order_relaxed);
		c = (r == 0);
	}
	assert(atomic_load_explicit(&y, memory_order_relaxed) == 1); /* 0 after an iteration */
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
	pthread_create(&a, NULL, waiter, (void *)0);
	pthread_create(&b, NULL, writer, NULL);
	return 0;
}
