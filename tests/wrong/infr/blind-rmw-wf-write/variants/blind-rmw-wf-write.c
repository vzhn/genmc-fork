#include <pthread.h>
#include <stdatomic.h>

/* Two identical blind increments and an increment whose result is used.  The
 * used one is a different kind of write to x, so the order the reduction keeps
 * for the blind ones may hide what it observes; the program must be reported. */
atomic_int x;
int r;

void *blind(void *arg)
{
	atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}

void *used(void *arg)
{
	r = atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t a, b, c;

	pthread_create(&a, NULL, blind, NULL);
	pthread_create(&b, NULL, blind, NULL);
	pthread_create(&c, NULL, used, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	pthread_join(c, NULL);
	return 0;
}
