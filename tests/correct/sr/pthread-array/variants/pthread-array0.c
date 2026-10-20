#include <pthread.h>
#include <stdatomic.h>

/* Workers spawned in a loop are symmetric: storing each handle into the
 * local array (which no worker can access) does not tell them apart */

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

	for (int i = 0; i < N; i++)
		pthread_create(&t[i], NULL, worker, NULL);
	for (int i = 0; i < N; i++)
		pthread_join(t[i], NULL);
	return 0;
}
