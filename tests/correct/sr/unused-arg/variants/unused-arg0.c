#include <pthread.h>
#include <stdatomic.h>

/* The workers get different arguments but never read them, so they are
 * still symmetric */

#define N 3

atomic_int x;

void *worker(void *unused)
{
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
