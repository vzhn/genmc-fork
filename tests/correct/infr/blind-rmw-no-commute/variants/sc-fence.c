#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

/* Any SC fence in the program disables the reduction (psc can order RMWs). */
atomic_int x;
int r;

void *t1(void *arg)
{
	atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}

void *t2(void *arg)
{
	atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t a, b;

	atomic_thread_fence(memory_order_seq_cst);
	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	return 0;
}
