#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <genmc.h>

/*
 * Listing 1.3 of Amir et al., "Deciding Serializability in Network Systems"
 * (TACAS 2026): the requests of wrong/serializability/ser-yield, under a
 * spinlock. They still interleave, but every conflict on X goes the way the
 * lock orders the requests: -check-serializability must accept every
 * execution. (Ser's check-and-set of L is atomic, hence the CAS.)
 */

atomic_int X;
atomic_int L;

void *request(void *unused)
{
	__VERIFIER_serial_begin();
	int unlocked = 0;
	while (!atomic_compare_exchange_strong_explicit(&L, &unlocked, 1, memory_order_acquire,
							memory_order_relaxed))
		unlocked = 0;

	atomic_store_explicit(&X, 1, memory_order_relaxed);
	int y = atomic_load_explicit(&X, memory_order_relaxed);
	atomic_store_explicit(&X, 0, memory_order_relaxed);

	atomic_store_explicit(&L, 0, memory_order_release);
	__VERIFIER_serial_end();
	return (void *)(long)y;
}

int main()
{
	pthread_t t1, t2;
	void *y1, *y2;

	if (pthread_create(&t1, NULL, request, NULL))
		abort();
	if (pthread_create(&t2, NULL, request, NULL))
		abort();
	if (pthread_join(t1, &y1))
		abort();
	if (pthread_join(t2, &y2))
		abort();

	assert(y1 == (void *)1 && y2 == (void *)1);
	return 0;
}
