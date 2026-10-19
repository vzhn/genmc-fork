#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

/* Two identical blind increments and a reader: the reader can see 0, 1 or 2.
 * Without the reduction, each value shows up under both increment orders. */
atomic_int x;
int r;

void *inc(void *arg)
{
	atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}

void *reader(void *arg)
{
	r = atomic_load_explicit(&x, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t a, b, c;

	pthread_create(&a, NULL, inc, NULL);
	pthread_create(&b, NULL, inc, NULL);
	pthread_create(&c, NULL, reader, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	pthread_join(c, NULL);
	assert(r >= 0 && r <= 2 && atomic_load_explicit(&x, memory_order_relaxed) == 2);
	return 0;
}
