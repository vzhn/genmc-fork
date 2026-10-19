#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>
#include <assert.h>

/*
 * A grace period of an srcu_struct waits for its readers: P1 sees P0's
 * second write before synchronize_srcu(), so the critical section began
 * before the grace period ended and P1 sees the first write after it.
 * herd7: Never, 3 executions.
 */

struct srcu_struct s;
int x;
int y;

int r1;
int r2;

void *P0(void *unused)
{
	int idx = srcu_read_lock(&s);
	WRITE_ONCE(x, 1);
	WRITE_ONCE(y, 1);
	srcu_read_unlock(&s, idx);
	return NULL;
}

void *P1(void *unused)
{
	r1 = READ_ONCE(y);
	synchronize_srcu(&s);
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

	assert(!(r1 == 1 && r2 == 0));
	return 0;
}
