#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>

/*
 * The branch takes a lock P1 may hold: when the revisited read makes P0
 * take it, P0 can block on the lock before the write the revisit kept (the
 * block goes after that write, and the run is dropped if P0 stays
 * blocked). herd7: Never, 3 executions; with P1's write to x inside its
 * critical section (LOCKED_WRITE) too.
 */

DEFINE_SPINLOCK(l);
int x, y;

void *P0(void *unused)
{
	int r0 = READ_ONCE(x);
	if (r0) {
		spin_lock(&l);
		spin_unlock(&l);
	}
	WRITE_ONCE(y, 1);
	return NULL;
}

void *P1(void *unused)
{
	spin_lock(&l);
	int r1 = READ_ONCE(y);
#ifdef LOCKED_WRITE
	WRITE_ONCE(x, r1);
	spin_unlock(&l);
#else
	spin_unlock(&l);
	WRITE_ONCE(x, r1);
#endif
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
