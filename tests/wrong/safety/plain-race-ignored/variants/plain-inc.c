#include <pthread.h>
#include <assert.h>

/*
 * With -disable-race-detection, the plain increments can lose an update:
 * both threads read 0 (#142)
 */

int x;

void *inc(void *unused)
{
	x++;
	return NULL;
}

int main()
{
	pthread_t a, b;

	pthread_create(&a, NULL, inc, NULL);
	pthread_create(&b, NULL, inc, NULL);
	pthread_join(a, NULL);
	pthread_join(b, NULL);
	assert(x == 2);
	return 0;
}
