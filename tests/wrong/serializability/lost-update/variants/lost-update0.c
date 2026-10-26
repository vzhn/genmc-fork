#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * Lost update: each region reads the counter and writes it back incremented.
 * When both read 0, each region's read is fr-before the other's write, a
 * cycle -check-serializability must report. (Compare
 * correct/serializability/counter-fai.)
 */

atomic_int counter;

void *increment(void *unused)
{
	__VERIFIER_serial_begin();
	int c = atomic_load_explicit(&counter, memory_order_relaxed);
	atomic_store_explicit(&counter, c + 1, memory_order_relaxed);
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
	return 0;
}
