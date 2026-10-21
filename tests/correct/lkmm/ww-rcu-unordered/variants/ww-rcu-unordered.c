/* 3 executions and the "Unordered writes" warning: the reader writes x
 * whichever pointer it found; when it found the new one its critical
 * section may follow the grace period, and nothing orders its write
 * against the updater's (issue #113, scripts/check-lkmm-ww-warning.sh). */
#include <pthread.h>
#include <lkmm.h>

int x;
int z;
int *y = &x;

void *reader(void *unused)
{
	rcu_read_lock();
	int *p = rcu_dereference(y);
	WRITE_ONCE(x, p == &x ? 1 : 3);
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
