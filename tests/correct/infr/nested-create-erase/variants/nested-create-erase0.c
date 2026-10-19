#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * Threads created by other threads, with a begin-only thread (t4) whose create
 * survives the revisit above a thread (t3) whose create is cut: the
 * clean-up of begins used to walk past t4 and erase t3's slot, shifting
 * t4 to another thread id (the runtime then failed thread.id ==
 * getNumThreads() when t4 was created again).
 */

atomic_int x;

static void *t3(void *unused) { return NULL; }
static void *t4(void *unused) { return NULL; }

static void *t1(void *unused)
{
	pthread_t t;
	pthread_create(&t, NULL, t3, NULL);
	return NULL;
}

static void *t2(void *unused)
{
	pthread_t t;
	pthread_create(&t, NULL, t4, NULL);
	atomic_store_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	(void)atomic_load_explicit(&x, memory_order_relaxed);
	return 0;
}
