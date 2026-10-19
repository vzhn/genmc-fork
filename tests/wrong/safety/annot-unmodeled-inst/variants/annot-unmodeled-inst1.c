#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * As annot-unmodeled-inst0, with an integer load converted to a double
 * (sitofp) before the comparison.
 */

atomic_int x;

static void *waiter(void *unused)
{
	while ((double)atomic_load_explicit(&x, memory_order_relaxed) < 0.5)
		;
	assert(0); /* reachable once the writer's store is read */
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
