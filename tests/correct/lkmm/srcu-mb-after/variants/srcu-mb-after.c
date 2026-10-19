#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>
#include <assert.h>

/*
 * smp_mb__after_srcu_read_unlock() after srcu_read_unlock() is a full
 * barrier: store buffering is forbidden. Without the barrier
 * (NO_MB_AFTER) it is allowed. herd7: Never (Sometimes without the
 * barrier), 3 executions (4 without it).
 */

struct srcu_struct s;
int x;
int y;

int r1;
int r2;

void *P0(void *unused)
{
	WRITE_ONCE(x, 1);
	int idx = srcu_read_lock(&s);
	srcu_read_unlock(&s, idx);
#ifndef NO_MB_AFTER
	smp_mb__after_srcu_read_unlock();
#endif
	r1 = READ_ONCE(y);
	return NULL;
}

void *P1(void *unused)
{
	WRITE_ONCE(y, 1);
	smp_mb();
	r2 = READ_ONCE(x);
	return NULL;
}

int main()
{
	pthread_t t0, t1;

	if (pthread_create(&t0, NULL, P0, NULL))
		abort();
	if (pthread_create(&t1, NULL, P1, NULL))
		abort();
	if (pthread_join(t0, NULL))
		abort();
	if (pthread_join(t1, NULL))
		abort();

#ifndef NO_MB_AFTER
	assert(!(r1 == 0 && r2 == 0));
#endif
	return 0;
}
