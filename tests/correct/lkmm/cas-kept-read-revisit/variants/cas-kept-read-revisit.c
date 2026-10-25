#include <pthread.h>
#include <stdatomic.h>

/*
 * Issue #237: the number of executions under -lkmm depended on the
 * scheduling policy (7 under wf, 5 or 6 under wfr seeds 3-7, arbitrary
 * seeds 4 and 6; herd7 allows 7, with READ_ONCE(), smp_store_release(),
 * cmpxchg_relaxed() and smp_load_acquire()). Under wfr seed 4, t2's CAS
 * was added before t1's first read and then revisited by t1's release
 * store, which made it succeed. t2's store to v0 then revisited t1's
 * first read: the revisit keeps t2's acquire read, which reads the CAS's
 * write part, so it keeps the CAS too (see keepRMWReads()), but the CAS
 * was kept already, as an event added before the read, and the closure did
 * not follow it to the release store it reads. The revisit was refused as
 * not closed, and no other path reached the two executions in which t1
 * reads t2's store and the CAS succeeds.
 */

atomic_int v0, v1;

void *t1(void *unused)
{
	atomic_load_explicit(&v0, memory_order_relaxed);
	atomic_load_explicit(&v0, memory_order_relaxed);
	atomic_store_explicit(&v1, 2, memory_order_release);
	return NULL;
}

void *t2(void *unused)
{
	int e = 2;
	atomic_compare_exchange_strong_explicit(&v1, &e, 3, memory_order_relaxed,
						memory_order_relaxed);
	atomic_load_explicit(&v1, memory_order_acquire);
	atomic_store_explicit(&v0, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t a, b;

	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	return 0;
}
