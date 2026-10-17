#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * A CAS retry loop whose initial value is also used after the loop, so the
 * initial load cannot be moved into the loop and the expected value stays in a
 * header PHI. A failed CAS changes the PHI, so its retry is not a repetition:
 * -check-liveness must not report the thread whose CAS failed on the latest
 * value (compare spin-cas-writeback-live).
 */

atomic_uint x;
atomic_uint seen[2];

static void *thread(void *arg)
{
	int id = (int)(intptr_t)arg;
	uint32_t old = atomic_load_explicit(&x, memory_order_relaxed);
	uint32_t first = old;
	while (!atomic_compare_exchange_strong_explicit(&x, &old, old + 1, memory_order_relaxed,
							 memory_order_relaxed))
		;
	atomic_store_explicit(&seen[id], old - first, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, thread, (void *)0);
	pthread_create(&b, NULL, thread, (void *)1);
	return 0;
}
