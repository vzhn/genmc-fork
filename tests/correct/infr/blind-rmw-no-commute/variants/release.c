#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

/* Release RMWs are synchronization sources; their order matters. */
atomic_int x;
int r;

void *t1(void *arg)
{
	atomic_fetch_add_explicit(&x, 1, memory_order_release);
	return NULL;
}

void *t2(void *arg)
{
	atomic_fetch_add_explicit(&x, 1, memory_order_release);
	return NULL;
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
