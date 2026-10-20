#include <pthread.h>
#include <assert.h>
#include <stdatomic.h>
#include <genmc.h>

/* Two unordered writes and an assume on a read of the location: under
 * in-place revisiting the unordered writes are an error, so GenMC used to
 * stop with "Unordered writes" before reaching the assertion. It now
 * starts over without IPR */

atomic_int x;

void *t1(void *unused)
{
	atomic_store_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}

void *t2(void *unused)
{
	atomic_store_explicit(&x, 2, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t a, b;

	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	__VERIFIER_assume(atomic_load_explicit(&x, memory_order_relaxed) == 2);
	assert(0);
	return 0;
}
