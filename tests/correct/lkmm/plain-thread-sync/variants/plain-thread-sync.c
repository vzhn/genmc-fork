#include <stdlib.h>
#include <lkmm.h>
#include <pthread.h>
#include <assert.h>

/*
 * Plain accesses ordered by thread creation and join only, also through
 * a chain of them (the first child's write, read by a child created after
 * the join), and through a join followed by a release. LKMM has no
 * threads; GenMC's pthread_create() and pthread_join() synchronise like a
 * release and an acquire, so none of these accesses race.
 */

int in;
int out;
int flag;

void *first(void *unused)
{
	out = in + 1;
	return NULL;
}

void *second(void *unused)
{
	assert(out == 42);
	return NULL;
}

void *third(void *unused)
{
	if (smp_load_acquire(&flag))
		assert(out == 42);
	return NULL;
}

int main()
{
	pthread_t t1, t2, t3;

	in = 41;
	pthread_create(&t1, NULL, first, NULL);
	pthread_create(&t3, NULL, third, NULL);
	pthread_join(t1, NULL);
	smp_store_release(&flag, 1);
	pthread_create(&t2, NULL, second, NULL);
	pthread_join(t2, NULL);
	pthread_join(t3, NULL);
	in = 0;
	return 0;
}
