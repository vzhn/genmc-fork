#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * A CAS retry loop whose initial load is acquire but whose CAS fails with a
 * relaxed read: the retry compares against a value read without
 * synchronization, and the read of data after it races with the write. The
 * spin-assume transformation may move the initial load into the loop only if
 * its ordering matches the CAS's failure ordering; with an acquire load at
 * the top of the loop the race would be hidden.
 */

atomic_uint x;
int data;

static void *thread(void *unused)
{
	uint32_t old = atomic_load_explicit(&x, memory_order_acquire);
	while (!atomic_compare_exchange_strong_explicit(&x, &old, old + 1, memory_order_release,
							 memory_order_relaxed))
		;
	if (old == 1) {
		int d = data; /* races unless the read of 1 synchronized */
		(void)d;
	}
	return NULL;
}

static void *writer(void *unused)
{
	data = 42;
	atomic_fetch_add_explicit(&x, 1, memory_order_release);
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, thread, NULL);
	pthread_create(&b, NULL, writer, NULL);
	return 0;
}
