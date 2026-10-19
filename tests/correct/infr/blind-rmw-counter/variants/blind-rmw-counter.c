#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

/* Four identical blind relaxed increments: 4! coherence orders, one behavior.
 * With --blind-rmw-reduction (RC11 only) a single execution is explored. */
#define N 4

atomic_int cnt;

void *worker(void *arg)
{
	atomic_fetch_add_explicit(&cnt, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t t[N];

	for (int i = 0; i < N; i++)
		pthread_create(&t[i], NULL, worker, NULL);
	for (int i = 0; i < N; i++)
		pthread_join(t[i], NULL);
	assert(atomic_load_explicit(&cnt, memory_order_relaxed) == N);
	return 0;
}
