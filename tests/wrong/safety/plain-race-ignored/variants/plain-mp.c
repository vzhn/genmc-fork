#include <pthread.h>
#include <assert.h>

/*
 * With -disable-race-detection, the racy executions are explored, plain
 * reads included: r can read flag == 1 and then data == 0 (data races
 * with both writes). A plain read used to read only the latest write, so
 * GenMC explored one execution and reported no errors (#142).
 */

int data, flag;

void *w(void *unused)
{
	data = 1;
	flag = 1;
	return NULL;
}

void *r(void *unused)
{
	if (flag)
		assert(data == 1);
	return NULL;
}

int main()
{
	pthread_t a, b;

	pthread_create(&a, NULL, w, NULL);
	pthread_create(&b, NULL, r, NULL);
	return 0;
}
