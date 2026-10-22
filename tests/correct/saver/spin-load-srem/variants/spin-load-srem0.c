/* A spin loop that waits for an even (signed) value: the read it blocks at
 * is annotated with a signed remainder, and is revisited in place (#92: the
 * annotation of a signed remainder was not computed, and each later write
 * explored a copy of the blocked execution) */

#include <pthread.h>
#include <stdatomic.h>

atomic_int seq;

void *waiter(void *arg)
{
	while (atomic_load_explicit(&seq, memory_order_acquire) % 2 == 1)
		;
	return NULL;
}

void *writer(void *arg)
{
	for (int i = 0; i < 2; i++) {
		atomic_fetch_add_explicit(&seq, 1, memory_order_relaxed);
		atomic_fetch_add_explicit(&seq, 1, memory_order_release);
	}
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	pthread_create(&t1, NULL, waiter, NULL);
	pthread_create(&t2, NULL, writer, NULL);
	return 0;
}
