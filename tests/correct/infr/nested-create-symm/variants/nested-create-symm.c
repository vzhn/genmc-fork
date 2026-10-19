#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>
/*
 * On the path where t1 reads z = 0 it creates t3 twice; after the revisit
 * that makes it read z = 1 the first create is cut, but the begin of its
 * thread stays (a later thread is non-empty) and is never revived. The
 * remaining create has the same function, argument and parent and no
 * memory access since the cut create's position, so symmetry reduction
 * used to pick the stale begin as the symmetric predecessor and merged its
 * out-of-date prefix view into the new thread (segfault). The arbitrary
 * schedules of args.*.mo.in trigger the revisit order in question.
 */
atomic_int z, r, y, w;
static void *t3(void *u) { atomic_fetch_add_explicit(&y, 1, memory_order_relaxed); return NULL; }
static void *t4(void *u) { return NULL; }
static void *t5(void *u) { atomic_store_explicit(&r, 1, memory_order_relaxed); return NULL; }
static void *t1(void *u)
{
	pthread_t t;
	int a = atomic_load_explicit(&z, memory_order_relaxed);
	int b = atomic_load_explicit(&r, memory_order_relaxed);
	if (!a)
		pthread_create(&t, NULL, t3, NULL);
	atomic_store_explicit(&w, b, memory_order_relaxed);
	pthread_create(&t, NULL, t3, NULL);
	return NULL;
}
static void *t2(void *u)
{
	pthread_t t;
	pthread_create(&t, NULL, t4, NULL);
	atomic_store_explicit(&z, 1, memory_order_relaxed);
	return NULL;
}
int main()
{
	pthread_t a, b, c;
	pthread_create(&c, NULL, t5, NULL);
	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	(void)atomic_load_explicit(&y, memory_order_relaxed);
	(void)atomic_load_explicit(&w, memory_order_relaxed);
	return 0;
}
