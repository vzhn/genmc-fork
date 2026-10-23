#include <pthread.h>
#include <stdatomic.h>

/* Under TSO, an RMW orders the stores before it (tso: po;[W] ends at its
 * write part). The write part of thread 1's fetch-and-add goes right after
 * the write its read part reads, thread 2's, and so co-before thread 2's
 * store of y: thread 2's store of x then cannot be co-before thread 1's
 *
 *   (2,x) ->mo (1,x) ->po (1,fai y) ->mo (2,y=1) ->po (2,x)
 *
 * The 12 executions are those of SC: no store here is followed by a load
 * of another location without an RMW in between. */
atomic_int x, y;

void *t0(void *arg)
{
	(void)atomic_load_explicit(&x, memory_order_relaxed);
	return NULL;
}

void *t1(void *arg)
{
	atomic_store_explicit(&x, 1, memory_order_relaxed);
	(void)atomic_fetch_add_explicit(&y, 1, memory_order_relaxed);
	return NULL;
}

void *t2(void *arg)
{
	(void)atomic_fetch_add_explicit(&y, 1, memory_order_relaxed);
	atomic_store_explicit(&y, 1, memory_order_relaxed);
	atomic_store_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t a, b, c;

	pthread_create(&a, NULL, t0, NULL);
	pthread_create(&b, NULL, t1, NULL);
	pthread_create(&c, NULL, t2, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	pthread_join(c, NULL);
	return 0;
}
