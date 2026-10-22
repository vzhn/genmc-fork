#include <pthread.h>
#include <assert.h>

/*
 * pthread_once() and thread-specific data: two threads set up a key with
 * pthread_once(), and each keeps its own value under it. The init routine
 * runs once, and neither thread returns from pthread_once() before it has
 * finished (the plain write to `ready` is not racy). A thread sees only
 * its own value, and the main thread none.
 */

pthread_once_t once = PTHREAD_ONCE_INIT;
pthread_key_t key;
int inits;
int ready;

void init(void)
{
	inits++;
	pthread_key_create(&key, NULL);
#ifdef BUG_ONCE_RECURSIVE
	pthread_once(&once, init);
#endif
	ready = 1;
}

void *thread(void *arg)
{
	pthread_once(&once, init);
	assert(ready == 1);
	assert(pthread_getspecific(key) == NULL);
	pthread_setspecific(key, arg);
	assert(pthread_getspecific(key) == arg);
	return NULL;
}

int main()
{
	pthread_t a, b;

	pthread_create(&a, NULL, thread, (void *) 1);
	pthread_create(&b, NULL, thread, (void *) 2);
	pthread_join(a, NULL);
	pthread_join(b, NULL);

	assert(inits == 1);
	assert(pthread_getspecific(key) == NULL);
	return 0;
}
