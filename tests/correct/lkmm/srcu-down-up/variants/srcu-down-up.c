#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>
#include <assert.h>

/*
 * A critical section that one thread opens (srcu_down_read()) and another
 * closes (srcu_up_read()), with the index passed through memory: the
 * unlock matches the lock by the data dependency of its value, through
 * the store and load of idx (the kernel model's carry-srcu-data). Both of
 * x and y are in the section, so the grace period orders them.
 * herd7 (with P1 reading idx = 1 as a filter): Never, 3 executions.
 */

struct srcu_struct s;
int x;
int y;
int idx;

int r2;
int r3;

void *P0(void *unused)
{
	int r0 = srcu_down_read(&s);
	WRITE_ONCE(x, 1);
	smp_store_release(&idx, r0 + 1);
	return NULL;
}

void *P1(void *unused)
{
	int r1 = smp_cond_load_acquire(&idx, VAL != 0);
	WRITE_ONCE(y, 1);
	srcu_up_read(&s, r1 - 1);
	return NULL;
}

void *P2(void *unused)
{
	r2 = READ_ONCE(y);
	synchronize_srcu(&s);
	r3 = READ_ONCE(x);
	return NULL;
}

int main()
{
	pthread_t t0, t1, t2;

	if (pthread_create(&t0, NULL, P0, NULL))
		abort();
	if (pthread_create(&t1, NULL, P1, NULL))
		abort();
	if (pthread_create(&t2, NULL, P2, NULL))
		abort();
	if (pthread_join(t0, NULL))
		abort();
	if (pthread_join(t1, NULL))
		abort();
	if (pthread_join(t2, NULL))
		abort();

	assert(!(r2 == 1 && r3 == 0));
	return 0;
}
