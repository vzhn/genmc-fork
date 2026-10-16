#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

// A loop (a CFG cycle) between the load and the assume.  The loop body touches
// no memory, so the path is clear; the search must terminate on the cycle.
atomic_int x;

void *writer(void *arg)
{
	atomic_store_explicit(&x, 1, memory_order_release);
	atomic_store_explicit(&x, 2, memory_order_release);
	return NULL;
}

int main()
{
	pthread_t t;

	pthread_create(&t, NULL, writer, NULL);
	int v = atomic_load_explicit(&x, memory_order_acquire);
	int acc = 0;
	for (int i = 0; i < v; i++)
		acc += i;
	__VERIFIER_assume(v == 2);
	assert(acc == 1);
	pthread_join(t, NULL);
	return 0;
}
