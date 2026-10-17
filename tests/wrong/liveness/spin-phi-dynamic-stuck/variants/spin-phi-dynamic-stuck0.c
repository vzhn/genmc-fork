#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * As wrong/safety/spin-phi-dynamic, but x never becomes 2: after reading the
 * final value the thread repeats the same iteration forever. Iterations that
 * change the header PHI must not count as repetitions, but this one does not
 * change it, and -check-liveness must still report the loop.
 */

atomic_uint x, z;

static void *thread(void *unused)
{
	int *done = malloc(sizeof(int));
	*done = 0;
	uint32_t r = atomic_load_explicit(&x, memory_order_relaxed);
	uint32_t first = r;
	for (;;) {
		if (r == 2)
			break;
		if (r == 1) {
			if (!*done) {
				atomic_store_explicit(&z, 1, memory_order_relaxed);
				*done = 1;
			}
			r = atomic_load_explicit(&x, memory_order_relaxed);
			continue;
		}
		r = atomic_load_explicit(&x, memory_order_relaxed);
	}
	atomic_store_explicit(&z, first, memory_order_relaxed);
	free(done);
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
	pthread_create(&a, NULL, thread, NULL);
	pthread_create(&b, NULL, writer, NULL);
	return 0;
}
