#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * A wait that can never finish: the waiter waits for flag == 1 and the other
 * thread never sets it. With -check-liveness this is a liveness violation
 * (an assume would only prune the execution).
 */

atomic_int flag;
atomic_int other;

void *waiter(void *unused)
{
	__VERIFIER_await(atomic_load_explicit(&flag, memory_order_acquire) == 1);
	return NULL;
}

void *forgetful(void *unused)
{
	atomic_store_explicit(&other, 1, memory_order_release);
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	if (pthread_create(&t1, NULL, waiter, NULL))
		abort();
	if (pthread_create(&t2, NULL, forgetful, NULL))
		abort();
	return 0;
}
