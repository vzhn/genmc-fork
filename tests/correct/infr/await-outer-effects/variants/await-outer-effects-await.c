#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <genmc.h>

/*
 * An outer retry loop with a side effect in every failed iteration (a retry
 * counter) and a wait inside it, in two threads contending for a lock.
 * Variants: the wait as __VERIFIER_await() and as __VERIFIER_assume(). Both
 * must explore the same executions (one expected count for both): an await
 * must not change the outer loop's side-effect tracking.
 */

#define WAIT(cond) __VERIFIER_await(cond)

atomic_int lock;
atomic_int retries;
int in_cs;

static void acquire(void)
{
	for (;;) {
		int unlocked = 0;

		if (atomic_compare_exchange_strong(&lock, &unlocked, 1))
			return;
		atomic_fetch_add_explicit(&retries, 1, memory_order_relaxed);
		WAIT(atomic_load(&lock) == 0);
	}
}

static void release(void) { atomic_store(&lock, 0); }

void *thread_n(void *unused)
{
	acquire();
	in_cs++;
	release();
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	if (pthread_create(&t1, NULL, thread_n, NULL))
		abort();
	if (pthread_create(&t2, NULL, thread_n, NULL))
		abort();
	return 0;
}
