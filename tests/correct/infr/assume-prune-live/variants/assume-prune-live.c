#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * __VERIFIER_assume() prunes: the executions where the reader sees flag == 0
 * are discarded, not reported, even under -check-liveness (args.*.in). Only
 * __VERIFIER_await() marks a wait.
 */

atomic_int flag;

void *reader(void *unused)
{
	__VERIFIER_assume(atomic_load_explicit(&flag, memory_order_acquire) == 1);
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

	if (pthread_create(&t1, NULL, reader, NULL))
		abort();
	if (pthread_create(&t2, NULL, setter, NULL))
		abort();
	return 0;
}
