/* As rmw-hole-kept-read.c, with a release store before: the RMW in the hole
 * can read only that store, which the kept cmpxchg reads already */
#include <lkmm.h>
#include <pthread.h>
#include <stddef.h>

int l, x;

void *t1(void *u)
{
	smp_store_release(&l, 0);
	if (READ_ONCE(x))
		cmpxchg_acquire(&l, 0, 1);
	cmpxchg_acquire(&l, 0, 1);
	return NULL;
}

void *t2(void *u)
{
	while (READ_ONCE(l) != 1)
		;
	WRITE_ONCE(x, 1);
	return NULL;
}

int main()
{
	pthread_t a, b;
	pthread_create(&a, NULL, t1, NULL);
	pthread_create(&b, NULL, t2, NULL);
	return 0;
}
