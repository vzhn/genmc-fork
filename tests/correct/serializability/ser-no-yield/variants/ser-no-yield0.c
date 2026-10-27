#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * Listing 1.1 of Amir et al., "Deciding Serializability in Network Systems"
 * (TACAS 2026): a request without a yield never interleaves with another.
 * The two requests run one after the other (the second thread is created
 * after the first is joined), so -check-serializability must accept every
 * execution. Compare wrong/serializability/ser-yield.
 */

atomic_int X;

void *request(void *unused)
{
	__VERIFIER_serial_begin();
	atomic_store_explicit(&X, 1, memory_order_relaxed);
	int y = atomic_load_explicit(&X, memory_order_relaxed);
	atomic_store_explicit(&X, 0, memory_order_relaxed);
	__VERIFIER_serial_end();
	return (void *)(long)y;
}

int main()
{
	pthread_t t1, t2;
	void *y1, *y2;

	if (pthread_create(&t1, NULL, request, NULL))
		abort();
	if (pthread_join(t1, &y1))
		abort();
	if (pthread_create(&t2, NULL, request, NULL))
		abort();
	if (pthread_join(t2, &y2))
		abort();

	assert(y1 == (void *)1 && y2 == (void *)1);
	return 0;
}
