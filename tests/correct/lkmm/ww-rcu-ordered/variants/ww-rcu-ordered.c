/* 2 executions and no "Unordered writes" warning: the reader writes x
 * only when it found the old pointer, so its critical section ends before
 * the grace period and its write is before the updater's (rcu-order, which
 * po and rf do not give); the kat's rule compares the writes against
 * GenMC's hb view (issue #113, scripts/check-lkmm-ww-warning.sh). */
#include <pthread.h>
#include <lkmm.h>

int x;
int z;
int *y = &x;

void *reader(void *unused)
{
	rcu_read_lock();
	int *p = rcu_dereference(y);
	if (p == &x)
		WRITE_ONCE(x, 1);
	rcu_read_unlock();
	return NULL;
}

void *updater(void *unused)
{
	rcu_assign_pointer(y, &z);
	synchronize_rcu();
	WRITE_ONCE(x, 2);
	return NULL;
}

int main()
{
	pthread_t r, u;

	pthread_create(&r, NULL, reader, NULL);
	pthread_create(&u, NULL, updater, NULL);
	pthread_join(r, NULL);
	pthread_join(u, NULL);
	return 0;
}
