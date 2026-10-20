#include <pthread.h>
#include <assert.h>
#include <stdatomic.h>

/* main spawns threads until one of them has run. An iteration that only
 * spawns a thread writes no memory, but it is no spinloop iteration: main
 * may spawn two threads before either runs, and the assertion fails.
 * (-unroll bounds the spawning) */

atomic_int n;

void *thread(void *unused)
{
	atomic_fetch_add(&n, 1);
	return NULL;
}

int main()
{
	pthread_t t;

	while (atomic_load(&n) == 0)
		pthread_create(&t, NULL, thread, NULL);
	assert(atomic_load(&n) < 2);
	return 0;
}
