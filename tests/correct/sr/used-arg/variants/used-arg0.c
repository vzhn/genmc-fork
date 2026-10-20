#include <pthread.h>
#include <stdatomic.h>

/* The workers read their (different) arguments: only the first one
 * increments, so they are not symmetric */

#define N 3

atomic_int x, y;

void *worker(void *arg)
{
	if ((long)arg == 0)
		atomic_fetch_add_explicit(&y, 1, memory_order_relaxed);
	atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t t[N];

	for (long i = 0; i < N; i++)
		pthread_create(&t[i], NULL, worker, (void *)i);
	for (int i = 0; i < N; i++)
		pthread_join(t[i], NULL);
	return 0;
}
