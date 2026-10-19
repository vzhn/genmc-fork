/*
 * Thread creation orders the parent's earlier events before the child's.
 * main reads x and then creates t2, which writes y; t1 copies y into x. main
 * cannot read 1 from x: the 1 comes from a thread created after the read.
 * Executions: t1 reads y = 0 (main reads x = 0 from the initialisation or
 * from t1), or y = 1 (main reads the initialisation): 3.
 *
 * Under -lkmm, nothing ordered main's read before the create (LKMM has no
 * thread creation), so the cycle was allowed (4 executions), and revisits
 * could copy a thread's events without its create, which failed an internal
 * check with -schedule-policy=arbitrary.
 */

#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>
#include <genmc.h>

int x;
int y;
int r;

void *thread_1(void *unused)
{
	int a = READ_ONCE(y);
	WRITE_ONCE(x, a);
	return NULL;
}

void *thread_2(void *unused)
{
	WRITE_ONCE(y, 1);
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	if (pthread_create(&t1, NULL, thread_1, NULL))
		abort();
	r = READ_ONCE(x);
	if (pthread_create(&t2, NULL, thread_2, NULL))
		abort();
	if (pthread_join(t1, NULL) || pthread_join(t2, NULL))
		abort();
	assert(r == 0);
	return 0;
}
