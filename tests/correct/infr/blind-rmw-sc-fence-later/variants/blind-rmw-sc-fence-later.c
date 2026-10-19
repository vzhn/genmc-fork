#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

/* An SC fence that no increment is hb-after cannot order them through psc,
 * so the reduction still applies (4! -> 1); compare blind-rmw-no-commute's
 * sc-fence variant, where the fence is hb-before both increments. */
#define N 4

atomic_int cnt;

void *worker(void *arg)
{
	atomic_fetch_add_explicit(&cnt, 1, memory_order_relaxed);
	atomic_thread_fence(memory_order_seq_cst);
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
