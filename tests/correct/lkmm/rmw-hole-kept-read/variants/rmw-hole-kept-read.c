/* -lkmm: a revisit makes t1 read x == 1 and keeps its second cmpxchg, which
 * t2's loop read reads. On the new path the first cmpxchg fills a hole
 * before the kept one: the only write it can read is the initial value,
 * which the kept cmpxchg reads already, and a successful RMW there breaks
 * atomicity. No completion is consistent (-lkmm-reference: 1 execution,
 * x == 0), and GenMC failed "!stores.empty()" instead of dropping
 * the run, with and without -disable-lkmm-exhaustive.
 */
#include <lkmm.h>
#include <pthread.h>
#include <stddef.h>

int l, x;

void *t1(void *u)
{
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
