#include <stdatomic.h>
#include <pthread.h>
#include <genmc.h>

/*
 * With race detection disabled, non-atomic labels (including kills) stay
 * in the replay schedule, so a thread kept in the graph with its kill is
 * replayed into the kill (see kill-error-replay for the default case).
 */

atomic_int x;
atomic_int y;

static void *reader(void *unused)
{
	(void)atomic_load_explicit(&y, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t t;
	pthread_create(&t, NULL, reader, NULL);
	atomic_store_explicit(&y, 1, memory_order_relaxed);
	while (atomic_load_explicit(&x, memory_order_relaxed) == 0)
		;
	pthread_join(t, NULL);
	return 0;
}
