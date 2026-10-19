#include <pthread.h>
#include <stdatomic.h>

/* t2's load is hb-after its own increments but not after t1's, so it can see
 * 2 with t1's increment still to come.  --blind-rmw-reduction keeps one order
 * of the increments and cannot show that; it must report the program. */
atomic_int x;

void *t1(void *arg)
{
	atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}

void *t2(void *arg)
{
	atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
	atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
	return (void *)(long)atomic_load_explicit(&x, memory_order_relaxed);
}

int main()
{
	pthread_t a, b;

	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	return 0;
}
