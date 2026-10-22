/* A spin loop whose condition reads two locations: both loads branch to the
 * latch, and the latch's failing assumption has to reach both, so that the
 * read of y, the one that blocks here, is revisited in place (#92: only the
 * first branch got it) */

#include <pthread.h>
#include <stdatomic.h>

atomic_int x, y, z;

void *waiter(void *arg)
{
	while (atomic_load_explicit(&x, memory_order_acquire) == 0 ||
	       atomic_load_explicit(&y, memory_order_acquire) == 0)
		;
	return NULL;
}

void *setter(void *arg)
{
	atomic_store_explicit(&x, 1, memory_order_release);
	/* (A read, so that the waiter runs before the stores to y) */
	atomic_load_explicit(&z, memory_order_relaxed);
	atomic_store_explicit(&y, 1, memory_order_release);
	atomic_store_explicit(&y, 2, memory_order_release);
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	pthread_create(&t1, NULL, waiter, NULL);
	pthread_create(&t2, NULL, setter, NULL);
	return 0;
}
