#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * The load annotator substituted TRUE for every instruction it could not
 * model (here the fcmp of the spin condition), so the annotation of the
 * load became assume(!true) = false and IPR blocked the read on every
 * value: the loop never exited and the assertion was never reached.
 * Such annotations are now dropped.
 */

_Atomic float x;

static void *waiter(void *unused)
{
	while (atomic_load_explicit(&x, memory_order_relaxed) == 0.0f)
		;
	assert(0); /* reachable once the writer's store is read */
	return NULL;
}

static void *writer(void *unused)
{
	atomic_store_explicit(&x, 1.0f, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, waiter, NULL);
	pthread_create(&b, NULL, writer, NULL);
	return 0;
}
