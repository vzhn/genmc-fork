#include <stdlib.h>
#include <lkmm.h>
#include <pthread.h>
#include <assert.h>

/*
 * herd7's gp = po ; [Sync-rcu] ; po? also ends at the synchronize_rcu()
 * itself, and a po-unlock-lock-po can start there, so herd7 has the pb
 * cycle r0 -fre-> x=1 -rfe-> r1 ; po ; [synchronize_rcu] ; po ; [unlock]
 * -rfe-> [lock] ; po ; r2 -mb-> z=1 -rfe-> r3 -rmb-> r0. GenMC narrows gp
 * to [M] ; po ; [Sync-rcu] ; po ; [M] and reaches the same verdict through
 * gp from r1 to the unlock and hb on from it (the lock read is an
 * acquire). herd7: Never, 7 executions.
 */

int x, z, a;
spinlock_t l;
int r0, r1, r2, r3;

void *P0(void *unused)
{
	r3 = READ_ONCE(z);
	smp_rmb();
	r0 = READ_ONCE(x);
	return NULL;
}

void *P1(void *unused)
{
	WRITE_ONCE(x, 1);
	return NULL;
}

void *P2(void *unused)
{
	spin_lock(&l);
	r1 = READ_ONCE(x);
	WRITE_ONCE(a, 1);
	synchronize_rcu();
	spin_unlock(&l);
	return NULL;
}

void *P3(void *unused)
{
	spin_lock(&l);
	r2 = READ_ONCE(a);
	smp_mb();
	WRITE_ONCE(z, 1);
	return NULL;
}

int main()
{
	pthread_t t0, t1, t2, t3;

	pthread_create(&t0, NULL, P0, NULL);
	pthread_create(&t1, NULL, P1, NULL);
	pthread_create(&t2, NULL, P2, NULL);
	pthread_create(&t3, NULL, P3, NULL);
	pthread_join(t0, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	pthread_join(t3, NULL);

	assert(!(r3 == 1 && r0 == 0 && r1 == 1 && r2 == 1));
	return 0;
}
