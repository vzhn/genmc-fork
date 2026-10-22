#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * The writes of x are unordered, and so are the ones of y, which c reads
 * in an assume: in-place revisiting (IPR) cannot have unordered writes at
 * the location of such a read, and GenMC starts over without it. The
 * fetch_add() writes y after a's store, with a single place in coherence:
 * once the writes of x had been reported, its unordered write was not
 * checked any more, and IPR stayed on (#130).
 * scripts/check-unordered-writes-fallback.sh checks that GenMC starts over.
 */

atomic_int x, y;

void *a(void *unused)
{
	atomic_store_explicit(&x, 1, memory_order_relaxed);
	atomic_store_explicit(&y, 1, memory_order_relaxed);
	return NULL;
}

void *b(void *unused)
{
	atomic_store_explicit(&x, 2, memory_order_relaxed);
	atomic_fetch_add_explicit(&y, 1, memory_order_relaxed);
	return NULL;
}

void *c(void *unused)
{
	__VERIFIER_assume(atomic_load_explicit(&y, memory_order_relaxed) == 2);
	return NULL;
}

int main()
{
	pthread_t ta, tb, tc;

	pthread_create(&ta, NULL, a, NULL);
	pthread_create(&tb, NULL, b, NULL);
	pthread_create(&tc, NULL, c, NULL);
	return 0;
}
