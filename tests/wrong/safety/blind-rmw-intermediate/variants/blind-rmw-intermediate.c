#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

/* Two identical blind increments: a reader seeing the intermediate value 1 must
 * still be found with --blind-rmw-reduction. */
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
	assert(r != 1);
	return 0;
}
