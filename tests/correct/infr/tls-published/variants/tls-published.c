#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

/*
 * A pointer to a thread-local (issue #90): the owner publishes the address
 * of its copy, and the helper writes through it. The write reaches the
 * owner's copy, not the helper's own, and is synchronized by `done` (no
 * race). (Publishing a thread-local node is a common lock idiom.)
 */

static __thread int tlx;
_Atomic(int *) published;
atomic_int done;

void *owner(void *arg)
{
	atomic_store(&published, &tlx);
	while (!atomic_load(&done))
		;
	assert(tlx == 1); /* the helper wrote our tlx through the pointer */
	return NULL;
}

void *helper(void *arg)
{
	int *p;
	while (!(p = atomic_load(&published)))
		;
	assert(p != &tlx);
	*p = 1;
	assert(tlx == 0); /* and not its own */
	atomic_store(&done, 1);
	return NULL;
}

int main()
{
	pthread_t a, b;

	pthread_create(&a, NULL, owner, NULL);
	pthread_create(&b, NULL, helper, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	assert(tlx == 0);
	return 0;
}
