/* CAS retry loop with an effect-free inner loop between the CAS and the
 * latch: the backedge annotation DFS meets a cycle (sentinel: true).
 * A CAS that reads 5 fails and the loop spins */
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>
#define RLX memory_order_relaxed
atomic_int x;
void *t1(void *u) {
	int e = 0;
	while (!atomic_compare_exchange_strong_explicit(&x, &e, 1, RLX, RLX)) {
		for (int i = 0; i < 2; i++)
			;
		e = 0;
	}
	return NULL;
}
void *t2(void *u) { atomic_store_explicit(&x, 5, RLX); return NULL; }
int main()
{
	pthread_t a, b;

	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	return 0;
}
