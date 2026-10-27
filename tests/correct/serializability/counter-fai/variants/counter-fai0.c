#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * The increments of wrong/serializability/lost-update, each one atomic RMW:
 * the region is a single event, so -check-serializability must accept every
 * execution.
 */

atomic_int counter;

void *increment(void *unused)
{
	__VERIFIER_serial_begin();
	atomic_fetch_add_explicit(&counter, 1, memory_order_relaxed);
	__VERIFIER_serial_end();
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	if (pthread_create(&t1, NULL, increment, NULL))
		abort();
	if (pthread_create(&t2, NULL, increment, NULL))
		abort();
	if (pthread_join(t1, NULL))
		abort();
	if (pthread_join(t2, NULL))
		abort();

	assert(atomic_load_explicit(&counter, memory_order_relaxed) == 2);
	return 0;
}
