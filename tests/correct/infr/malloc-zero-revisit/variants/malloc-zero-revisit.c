#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

/*
 * A zero-size block that is live while other threads produce revisits.
 * Every execution after the first one rebuilds the allocation map from
 * the graph, and the block has to be registered again (with the byte of
 * address space malloc(0) takes) so that main can still free it: GenMC
 * used to drop it and abort on the free (issue #115).
 */

atomic_int x;

static void *t1(void *unused)
{
	atomic_store_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}

static void *t2(void *unused)
{
	atomic_store_explicit(&x, 2, memory_order_relaxed);
	return NULL;
}

int main()
{
	char *p = malloc(0);
	pthread_t a, b;

	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	free(p);
	return 0;
}
