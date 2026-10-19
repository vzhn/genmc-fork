#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

/* A release fence before a relaxed RMW makes it a synchronization source. */
atomic_int x;
int r;

void *t1(void *arg)
{
	atomic_thread_fence(memory_order_release);
	atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}

void *t2(void *arg)
{
	atomic_thread_fence(memory_order_release);
	atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
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
