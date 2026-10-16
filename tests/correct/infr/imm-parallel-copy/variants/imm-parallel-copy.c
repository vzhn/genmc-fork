#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>

/*
 * Many backward revisits, explored with several threads (args.*.in). Each
 * revisit copies the graph for another worker. Under IMM the labels' prefix
 * views are DepViews, whose storage the copy used to share with the original:
 * two workers then updated one non-atomic refcount and corrupted the heap.
 */

#define N 4

atomic_int x, y, z;

void *thread_n(void *unused)
{
	atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
	atomic_fetch_add_explicit(&y, 1, memory_order_relaxed);
	atomic_fetch_add_explicit(&z, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t t[N];

	for (int i = 0; i < N; i++)
		if (pthread_create(&t[i], NULL, thread_n, NULL))
			abort();
	for (int i = 0; i < N; i++)
		pthread_join(t[i], NULL);
	assert(x == N && y == N && z == N);
	return 0;
}
