#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * A loop that refreshes a loaded value at the end of each iteration carries it
 * in a header PHI, so an iteration that reads a new value is not repeated by
 * the next one. The spin-assume transformation pruned every iteration that
 * returned to the header, so the loop never exited with a value other than the
 * initial one, and the assertion was never reached with r != first.
 * (The code condenser used to hide this case too; see condenser-reused-value.)
 */

atomic_uint x;

static void *thread(void *unused)
{
	uint32_t r = atomic_load_explicit(&x, memory_order_relaxed);
	uint32_t first = r;
	while (r == 0)
		r = atomic_load_explicit(&x, memory_order_relaxed);
	assert(r == first); /* fails iff the loop iterated */
	return NULL;
}

static void *writer(void *unused)
{
	atomic_store_explicit(&x, 1, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, thread, NULL);
	pthread_create(&b, NULL, writer, NULL);
	return 0;
}
