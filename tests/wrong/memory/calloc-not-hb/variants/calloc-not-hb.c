#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>

/*
 * A calloc()ed block counts as initialized by the allocation event, so a
 * reader that is not hb-after that event (the pointer travels through a
 * relaxed store) still reads uninitialized memory.
 */

static int *_Atomic shared;

void *thread_1(void *unused)
{
	int *q = calloc(4, sizeof(int));

	atomic_store_explicit(&shared, q, memory_order_relaxed);
	return NULL;
}

void *thread_2(void *unused)
{
	int *q = atomic_load_explicit(&shared, memory_order_relaxed);

	if (q) {
		assert(q[2] == 0);
		free(q);
	}
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	pthread_create(&t1, NULL, thread_1, NULL);
	pthread_create(&t2, NULL, thread_2, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	return 0;
}
