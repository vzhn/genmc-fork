#include <pthread.h>
#include <assert.h>

/*
 * With -disable-race-detection, the racy executions are explored, plain
 * accesses as atomic ones: each thread's plain read can read either the
 * other thread's write or the initial value (store buffering), and the
 * plain increments of z take any order. A plain read used to read only
 * the latest write, and a plain write to go last, so GenMC explored one
 * execution (#142).
 */

int x, y, z;
int r1, r2;

void *a(void *unused)
{
	x = 1;
	r1 = y;
	z++;
	return NULL;
}

void *b(void *unused)
{
	y = 1;
	r2 = x;
	z++;
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	pthread_create(&t1, NULL, a, NULL);
	pthread_create(&t2, NULL, b, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	assert(z >= 1 && z <= 2);
	return 0;
}
