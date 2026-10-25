#include <pthread.h>
#include <stdatomic.h>

/*
 * Issue #237: -lkmm counted 508 executions under wf, ltr and arbitrary
 * seeds 1 and 2, 480 under wfr seed 1 and 499 under seed 4. herd7 allows
 * 480 (with READ_ONCE(), atomic_fetch_add_relaxed(), WRITE_ONCE() and
 * smp_load_acquire()). The 28 extra executions broke atomicity: a store
 * executed anew after a revisit went between t1's fetch-and-add's write
 * and the write its read reads (see rmw-placing-range-end).
 */

atomic_int v0, v1;

void *t1(void *unused)
{
	atomic_load_explicit(&v0, memory_order_relaxed);
	atomic_load_explicit(&v0, memory_order_relaxed);
	atomic_load_explicit(&v0, memory_order_relaxed);
	atomic_fetch_add_explicit(&v1, 1, memory_order_relaxed);
	return NULL;
}

void *t2(void *unused)
{
	atomic_load_explicit(&v0, memory_order_relaxed);
	atomic_store_explicit(&v1, 1, memory_order_relaxed);
	atomic_load_explicit(&v1, memory_order_acquire);
	atomic_store_explicit(&v0, 1, memory_order_relaxed);
	return NULL;
}

void *t3(void *unused)
{
	atomic_load_explicit(&v0, memory_order_relaxed);
	atomic_store_explicit(&v0, 1, memory_order_relaxed);
	atomic_store_explicit(&v1, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t a, b, c;

	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	pthread_create(&c, NULL, t3, NULL);
	return 0;
}
