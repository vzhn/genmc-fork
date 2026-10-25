#include <pthread.h>
#include <stdatomic.h>

/*
 * Issue #237 (found by ScriptedDriverMatrixLarge.DISABLED_LKMM): 25
 * executions under ltr and most arbitrary seeds, 24 under wf and wfr; herd7
 * allows 24 (with READ_ONCE(), atomic_fetch_add_relaxed(), WRITE_ONCE() and
 * smp_load_acquire()). The revisit of t1's read by t3's store to v0 keeps
 * t3's acquire read, which reads t2's fetch-and-add, and deletes t3's first
 * store. Executed anew, that store must come before the fetch-and-add's
 * write in coherence (the acquire read reads it), and the placings of the
 * checker's range ended with the one right before it: between the
 * fetch-and-add's write and t4's store, which its read reads (atomicity).
 */

atomic_int v0, v1;

void *t1(void *unused)
{
	atomic_load_explicit(&v0, memory_order_relaxed);
	return NULL;
}

void *t2(void *unused)
{
	atomic_fetch_add_explicit(&v1, 1, memory_order_relaxed);
	return NULL;
}

void *t3(void *unused)
{
	atomic_store_explicit(&v1, 1, memory_order_relaxed);
	atomic_load_explicit(&v0, memory_order_relaxed);
	atomic_load_explicit(&v1, memory_order_acquire);
	atomic_store_explicit(&v0, 1, memory_order_relaxed);
	return NULL;
}

void *t4(void *unused)
{
	atomic_store_explicit(&v1, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t a, b, c, d;

	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	pthread_create(&c, NULL, t3, NULL);
	pthread_create(&d, NULL, t4, NULL);
	return 0;
}
