#include <pthread.h>
#include <stdatomic.h>

/*
 * a and b write x unordered, an error under symmetry reduction once the
 * graph has symmetric threads (GenMC then starts over without SR); the
 * readers are symmetric, and write nothing. Which comes first, the
 * unordered writes or the creation of the second reader, depends on the
 * schedule: under most arbitrary schedules a and b ran first, SR stayed
 * on, and 12 executions were explored instead of 18 (#130).
 * scripts/check-unordered-writes-fallback.sh runs it under several.
 */

atomic_int x;

void *a(void *unused)
{
	atomic_store_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}

void *b(void *unused)
{
	atomic_store_explicit(&x, 2, memory_order_relaxed);
	return NULL;
}

void *reader(void *unused)
{
	(void)atomic_load_explicit(&x, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t ta, tb, t[2];

	pthread_create(&ta, NULL, a, NULL);
	pthread_create(&tb, NULL, b, NULL);
	for (int i = 0; i < 2; i++)
		pthread_create(&t[i], NULL, reader, NULL);
	return 0;
}
