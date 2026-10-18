#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * As condenser-reused-value0, with a CAS loop: the reload after a failed CAS
 * is bisimilar to the initial load, whose value escapes to the assertion.
 */

atomic_int x;

static void *incrementer(void *unused)
{
	int old = atomic_load_explicit(&x, memory_order_relaxed);
	int first = old;
	for (;;) {
		int exp = old;
		if (atomic_compare_exchange_strong_explicit(&x, &exp, old + 1,
							    memory_order_relaxed,
							    memory_order_relaxed))
			break;
		old = atomic_load_explicit(&x, memory_order_relaxed);
	}
	assert(old == first); /* fails iff the CAS failed once */
	return NULL;
}

static void *adder(void *unused)
{
	atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, incrementer, NULL);
	pthread_create(&b, NULL, adder, NULL);
	return 0;
}
