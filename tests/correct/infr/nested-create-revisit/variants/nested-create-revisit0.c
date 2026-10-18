#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * Threads created by other threads, with a revisit that cuts a create
 * event but not the begin of its thread while a later thread stays
 * non-empty. Such a begin cannot be removed (thread ids are positional),
 * so GenMC used to keep it attached to a create that was cut, and failed
 * an internal check when the create was executed again. Under IMM the
 * create of t3 is outside the (dependency-tracking) prefix of t4's write
 * to x; under the LTR schedule fixed by args.*.mo.in the same shape
 * arises from stamps.
 */

atomic_int x;
atomic_int y;

static void *t3(void *unused) { return NULL; }

static void *t4(void *unused)
{
	int a = atomic_load_explicit(&y, memory_order_relaxed);
	atomic_store_explicit(&x, a, memory_order_relaxed);
	return NULL;
}

static void *t1(void *unused)
{
	pthread_t t;
	pthread_create(&t, NULL, t3, NULL);
	atomic_store_explicit(&y, 1, memory_order_relaxed);
	return NULL;
}

static void *t2(void *unused)
{
	pthread_t t;
	pthread_create(&t, NULL, t4, NULL);
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
