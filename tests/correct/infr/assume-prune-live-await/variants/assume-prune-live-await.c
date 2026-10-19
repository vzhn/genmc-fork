#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * An execution pruned by __VERIFIER_assume() is discarded as a whole, even
 * when another thread's __VERIFIER_await() is stuck on the pruned thread:
 * when the assumer reads coin == 1 it never sets flag, and the waiter's
 * await never ends, but that execution is infeasible, so under
 * -check-liveness (args.*.in) it is not a liveness violation.
 */

atomic_int flag, coin;

void *assumer(void *unused)
{
	int c = atomic_load_explicit(&coin, memory_order_relaxed);
	__VERIFIER_assume(c == 0);
	atomic_store_explicit(&flag, 1, memory_order_release);
	return NULL;
}

void *waiter(void *unused)
{
	__VERIFIER_await(atomic_load_explicit(&flag, memory_order_acquire));
	return NULL;
}

void *tosser(void *unused)
{
	atomic_store_explicit(&coin, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t t1, t2, t3;

	if (pthread_create(&t1, NULL, tosser, NULL))
		abort();
	if (pthread_create(&t2, NULL, assumer, NULL))
		abort();
	if (pthread_create(&t3, NULL, waiter, NULL))
		abort();
	return 0;
}
