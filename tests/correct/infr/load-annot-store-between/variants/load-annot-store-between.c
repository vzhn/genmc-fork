#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

// A store on every path between the load and the assume: the load must not be
// annotated, and the store stays visible to the other thread.
atomic_int x;
atomic_int seen;

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
	atomic_store_explicit(&seen, v, memory_order_relaxed);
	if (v > 100)
		atomic_store_explicit(&seen, 0, memory_order_relaxed);
	else
		atomic_store_explicit(&seen, v + 1, memory_order_relaxed);
	__VERIFIER_assume(v == 2);
	assert(atomic_load_explicit(&seen, memory_order_relaxed) == 3);
	pthread_join(t, NULL);
	return 0;
}
