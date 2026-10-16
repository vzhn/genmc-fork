#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

// Between the load and the assume one branch stores and the other is clear.
// Some path is clear, so the load is annotatable; the result must not change.
atomic_int x;
int g;

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
	if (v == 1)
		g = 1;
	__VERIFIER_assume(v != 0);
	assert(v == 1 || v == 2);
	pthread_join(t, NULL);
	return 0;
}
