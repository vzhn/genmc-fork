#include <pthread.h>
#include <stdatomic.h>

/*
 * The write of a fetch-and-add is computed from its read, a data dependency
 * herd7 has (iico_data) and GenMC does not record: through the rfi to t1's
 * acquire read, it orders the fetch-and-add's read before that read (LKMM's
 * to-r = dep ; [Marked] ; rfi), so before t1's store to v1. The fetch-and-add
 * cannot then read t2's release store while t2 reads t1's store to v1 (an
 * hb cycle): herd7 allows 7 executions. -lkmm counted that one too under
 * some schedules (8 under -schedule-policy=wfr -schedule-seed=4, and under
 * -disable-lkmm-exhaustive with every policy), 7 under the others (#237).
 */

atomic_int v0, v1;

void *t1(void *unused)
{
	atomic_fetch_add_explicit(&v0, 1, memory_order_relaxed);
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
