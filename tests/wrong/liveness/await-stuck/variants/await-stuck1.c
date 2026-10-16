#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * A wait on two variables that can never finish: the other thread sets one
 * of them but not the other.
 */

atomic_int flag;
atomic_int other;

void *waiter(void *unused)
{
	__VERIFIER_await(atomic_load_explicit(&flag, memory_order_acquire) == 1 &&
			 atomic_load_explicit(&other, memory_order_acquire) == 1);
	return NULL;
}

void *forgetful(void *unused)
{
	atomic_store_explicit(&flag, 1, memory_order_release);
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
