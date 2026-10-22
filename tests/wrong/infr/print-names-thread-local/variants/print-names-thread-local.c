#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

/*
 * The names printed for the fields of a thread-local aggregate follow its
 * declaration, as for a global, and an access to another thread's copy
 * names that thread (issue #90)
 */

struct s {
	int x;
	int a[2];
	long y;
};

static __thread struct s tl = {1, {2, 3}, 4};
_Atomic(struct s *) published;
atomic_int done;

void *thread_1(void *unused)
{
	tl.a[1] = 5;
	atomic_store(&published, &tl);
	while (!atomic_load(&done))
		;
	return NULL;
}

void *thread_2(void *unused)
{
	struct s *p = atomic_load(&published);
	if (p && p->a[1] == 5 && p->y == 4 && tl.a[1] == 3)
		assert(0);
	atomic_store(&done, 1);
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	pthread_create(&t1, NULL, thread_1, NULL);
	pthread_create(&t2, NULL, thread_2, NULL);
	return 0;
}
