#include <pthread.h>
#include <stdatomic.h>

/*
 * The parent's thread-local is published, and the parent writes it between
 * two spawns (issue #90): only the first worker can read the old value, so
 * the workers are not symmetric. (A thread-local is not private to its
 * thread, unlike a local that does not escape)
 */

static __thread atomic_int tl;
atomic_int *_Atomic pub;
atomic_int x;

void *worker(void *unused)
{
	atomic_int *p = atomic_load_explicit(&pub, memory_order_relaxed);
	if (atomic_load_explicit(p, memory_order_relaxed) != 0)
		atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
	else
		atomic_store_explicit(&x, 42, memory_order_relaxed);
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	atomic_store_explicit(&pub, &tl, memory_order_relaxed);
	pthread_create(&t1, NULL, worker, NULL);
	atomic_store_explicit(&tl, 1, memory_order_relaxed);
	pthread_create(&t2, NULL, worker, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	return 0;
}
