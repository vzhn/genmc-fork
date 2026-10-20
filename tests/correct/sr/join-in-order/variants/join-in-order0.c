#include <pthread.h>
#include <stdatomic.h>

/* Symmetric workers joined in spawn order, with reads in between: symmetry
 * reduction still applies */

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
	int seen = 0;

	for (int i = 0; i < N; i++)
		pthread_create(&t[i], NULL, worker, NULL);
	for (int i = 0; i < N; i++) {
		pthread_join(t[i], NULL);
		seen += atomic_load_explicit(&x, memory_order_relaxed);
	}
	return seen < 0;
}
