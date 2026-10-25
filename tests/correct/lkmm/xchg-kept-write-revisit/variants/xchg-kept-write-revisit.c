#include <pthread.h>
#include <stdatomic.h>

/*
 * The xchg writes its argument, whatever it reads: no dependency orders its
 * read before t1's acquire read of its write, and herd7 allows the
 * execution where the xchg reads t2's release store while t2 reads t1's
 * store to v1 (8 executions). Under the wf and ltr schedules, -lkmm builds
 * it only by the revisit of the xchg by t2's store, which keeps t1's acquire
 * read (t2's read of v1 depends on t1's store, which depends on it), and the
 * acquire read reads the xchg's write part: the revisit was not taken, and
 * -lkmm counted 7 (#237). The write part, which reading t2's store leaves as
 * it is, now stays, and moves right after that store in coherence.
 */

atomic_int v0, v1;

void *t1(void *unused)
{
	atomic_exchange_explicit(&v0, 2, memory_order_relaxed);
	atomic_load_explicit(&v0, memory_order_acquire);
	atomic_store_explicit(&v1, 1, memory_order_relaxed);
	return NULL;
}

void *t2(void *unused)
{
	atomic_load_explicit(&v1, memory_order_relaxed);
	atomic_load_explicit(&v0, memory_order_relaxed);
	atomic_store_explicit(&v0, 1, memory_order_release);
	return NULL;
}

int main()
{
	pthread_t a, b;

	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	return 0;
}
