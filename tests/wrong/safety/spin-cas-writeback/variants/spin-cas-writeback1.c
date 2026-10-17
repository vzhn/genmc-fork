#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * As spin-cas-writeback0, but the initial load is used only by the loop.
 * Pruning the retry is still unsound: the other thread adds 5 only after it
 * sees the release store, which comes after the initial load, so the initial
 * load cannot read 5 and only a retried CAS ends with old == 5.
 */

atomic_uint x;
atomic_int y;

static void *thread(void *unused)
{
	uint32_t old = atomic_load_explicit(&x, memory_order_relaxed);
	atomic_store_explicit(&y, 1, memory_order_release);
	while (!atomic_compare_exchange_strong_explicit(&x, &old, old + 1, memory_order_relaxed,
							 memory_order_relaxed))
		;
	assert(old != 5);
	return NULL;
}

static void *other(void *unused)
{
	if (atomic_load_explicit(&y, memory_order_acquire) == 1)
		atomic_fetch_add_explicit(&x, 5, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, thread, NULL);
	pthread_create(&b, NULL, other, NULL);
	return 0;
}
