#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * A CAS retry loop terminates: a failed compare_exchange writes the value it
 * read into its expected argument, and the retry compares against it. The
 * spin-assume transformation took the loop for a spinloop, and -check-liveness
 * reported the thread whose CAS failed on the latest value as non-terminating.
 */

atomic_uint x;

static void *thread(void *unused)
{
	uint32_t old = atomic_load_explicit(&x, memory_order_relaxed);
	while (!atomic_compare_exchange_strong_explicit(&x, &old, old + 1, memory_order_relaxed,
							 memory_order_relaxed))
		;
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, thread, NULL);
	pthread_create(&b, NULL, thread, NULL);
	return 0;
}
