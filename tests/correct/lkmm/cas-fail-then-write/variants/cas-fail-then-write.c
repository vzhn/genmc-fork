#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>

/*
 * A relaxed cmpxchg() that fails, then a write that does not depend on it,
 * read by P1, which makes the cmpxchg() succeed (herd7: 4 executions, one
 * LB). The revisit adds the cmpxchg()'s write where the write it kept was.
 */

int sl = 1, x0;

void *P0(void *unused)
{
	(void) cmpxchg_relaxed(&sl, 0, 1);
	WRITE_ONCE(x0, 1);
	return NULL;
}

void *P1(void *unused)
{
	int r = READ_ONCE(x0);
	WRITE_ONCE(sl, r - 1);
	return NULL;
}

int main()
{
	pthread_t t0, t1;

	if (pthread_create(&t0, NULL, P0, NULL))
		abort();
	if (pthread_create(&t1, NULL, P1, NULL))
		abort();
	return 0;
}
