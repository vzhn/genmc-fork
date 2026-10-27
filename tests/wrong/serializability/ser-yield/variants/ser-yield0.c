#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

/*
 * Listing 1.2 of Amir et al., "Deciding Serializability in Network Systems"
 * (TACAS 2026): two concurrent requests interleave, and one can read 0, which
 * no serial run returns. -check-serializability must report the cycle between
 * the two regions (e.g., A writes 1, B writes 1, reads 1 and writes 0, A reads
 * B's 0). Compare correct/serializability/ser-no-yield and ser-spinlock.
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

	if (pthread_create(&t1, NULL, request, NULL))
		abort();
	if (pthread_create(&t2, NULL, request, NULL))
		abort();
	if (pthread_join(t1, NULL))
		abort();
	if (pthread_join(t2, NULL))
		abort();
	return 0;
}
