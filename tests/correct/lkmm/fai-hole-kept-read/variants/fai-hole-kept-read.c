#include <pthread.h>
#include <stdatomic.h>

/*
 * The revisit of t1's read by t2's store to v1 keeps t2's acquire read of
 * v0, which reads t1's store, and not t2's fetch-and-add before it. The
 * fetch-and-add, executed anew into the hole, could read t1's store: its
 * write then went right after that store in coherence, and the kept read
 * read a write co-before one its thread wrote before it (CoWR). -lkmm
 * counted that incoherent graph: 7 executions, herd7 allows 6 (the
 * litmus version, with atomic_fetch_add_relaxed() and
 * atomic_read_acquire()).
 *
 * The optimal exploration (-disable-lkmm-exhaustive) found 4: it took
 * t1's store, read by the kept read, as an alternative for the
 * fetch-and-add deleted by the revisit (which rejected the revisit as
 * non-maximal), and it did not let t1's store revisit the fetch-and-add
 * once executed anew, its own kept write and the read of it being after
 * it in its thread.
 */

atomic_int v0, v1;

void *t1(void *unused)
{
	atomic_load_explicit(&v1, memory_order_relaxed);
	atomic_store_explicit(&v0, 1, memory_order_relaxed);
	return NULL;
}

void *t2(void *unused)
{
	atomic_fetch_add_explicit(&v0, 1, memory_order_relaxed);
	atomic_load_explicit(&v0, memory_order_acquire);
	atomic_store_explicit(&v1, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t a, b;

	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	return 0;
}
