#include <pthread.h>
#include <stdatomic.h>

/*
 * A race through a pointer to a thread-local (issue #90): the owner
 * publishes the address of its copy and writes the copy again, while the
 * helper writes through the pointer (before the owner ends). Accesses to
 * a thread-local are accesses to memory: the two writes race.
 */

static __thread int tlx;
_Atomic(int *) published;
atomic_int done;

void *owner(void *arg)
{
	atomic_store_explicit(&published, &tlx, memory_order_release);
	tlx = 2;
	while (!atomic_load_explicit(&done, memory_order_acquire))
		;
	return NULL;
}

void *helper(void *arg)
{
	int *p = atomic_load_explicit(&published, memory_order_acquire);
	if (p)
		*p = 1;
	atomic_store_explicit(&done, 1, memory_order_release);
	return NULL;
}

int main()
{
	pthread_t a, b;

	pthread_create(&a, NULL, owner, NULL);
	pthread_create(&b, NULL, helper, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	return 0;
}
