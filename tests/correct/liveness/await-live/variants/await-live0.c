#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * A wait that finishes: the other thread sets the flag. -check-liveness must
 * not report it (compare wrong/liveness/await-stuck).
 */

atomic_int flag;
atomic_int other;

void *waiter(void *unused)
{
	__VERIFIER_await(atomic_load_explicit(&flag, memory_order_acquire) == 1);
	return NULL;
}

void *setter(void *unused)
{
	atomic_store_explicit(&flag, 1, memory_order_release);
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	if (pthread_create(&t1, NULL, waiter, NULL))
		abort();
	if (pthread_create(&t2, NULL, setter, NULL))
		abort();
	return 0;
}
