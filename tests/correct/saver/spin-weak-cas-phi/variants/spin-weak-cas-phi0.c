#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * A weak-CAS retry loop whose initial value is also used after the loop, so the
 * expected value stays in a header PHI. A spurious failure leaves the PHI
 * unchanged and must still be pruned (the loop would otherwise be unbounded);
 * a real failure changes it, and its retry must be explored.
 */

atomic_uint x;

static void *thread(void *unused)
{
	uint32_t old = atomic_load_explicit(&x, memory_order_relaxed);
	uint32_t first = old;
	while (!atomic_compare_exchange_weak_explicit(&x, &old, old + 1, memory_order_relaxed,
						      memory_order_relaxed))
		;
	assert(old >= first);
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, thread, NULL);
	pthread_create(&b, NULL, thread, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	assert(atomic_load_explicit(&x, memory_order_relaxed) == 2);
	return 0;
}
