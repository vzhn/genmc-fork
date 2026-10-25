#include <pthread.h>
#include <stdatomic.h>

/*
 * Under IMM, the default exploration missed two executions under every
 * schedule but some arbitrary ones (100 of 102). In them, t2's first read
 * of v0 reads t3's store, its fetch-and-add reads t0's store, and t3's
 * fetch-and-add reads t2's store to v1: load buffering through t2's
 * first read, which nothing orders.
 *
 * They come from the revisit of t2's first read by t3's store to v0,
 * which keeps t2's fetch-and-add (t3's fetch-and-add reads t2's store to
 * v1, ordered after it). The fetch-and-add bounds what the first read may
 * read; the first read read t0's store, the maximal write within that
 * bound, but not the maximal one when it was added (t1's store was then
 * co-after it), and such a read was no candidate to revisit.
 */

atomic_int v0, v1;

void *t0(void *unused)
{
	atomic_store_explicit(&v0, 1, memory_order_relaxed);
	return NULL;
}

void *t1(void *unused)
{
	atomic_store_explicit(&v0, 1, memory_order_relaxed);
	return NULL;
}

void *t2(void *unused)
{
	atomic_load_explicit(&v0, memory_order_relaxed);
	atomic_fetch_add_explicit(&v0, 1, memory_order_relaxed);
	atomic_store_explicit(&v1, 1, memory_order_relaxed);
	return NULL;
}

void *t3(void *unused)
{
	atomic_fetch_add_explicit(&v1, 1, memory_order_relaxed);
	atomic_store_explicit(&v0, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t t[4];

	pthread_create(&t[0], NULL, t0, NULL);
	pthread_create(&t[1], NULL, t1, NULL);
	pthread_create(&t[2], NULL, t2, NULL);
	pthread_create(&t[3], NULL, t3, NULL);
	return 0;
}
