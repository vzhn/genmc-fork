/* CAS retry loop that may also leave on a loaded flag after the failed CAS:
 * the backedge annotation has a register the evaluator does not know, and
 * the break leaves the backedge paths (sentinel: false) */
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>
#define RLX memory_order_relaxed
atomic_int x; atomic_int stop;
void *t1(void *u) {
	int e = 0;
	while (!atomic_compare_exchange_strong_explicit(&x, &e, 1, RLX, RLX)) {
		if (atomic_load_explicit(&stop, RLX))
			break;
		e = 0;
	}
	return NULL;
}
void *t2(void *u) { atomic_store_explicit(&x, 5, RLX); atomic_store_explicit(&stop, 1, RLX); return NULL; }
int main()
{
	pthread_t a, b;

	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	return 0;
}
