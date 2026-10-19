#include <pthread.h>
#include <assert.h>

/*
 * pthread condition variables: two consumers wait, in a loop, for the items
 * two producers add. A consumer is never left waiting (-check-liveness, in
 * args.*.in) and never takes an item that is not there.
 */

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c = PTHREAD_COND_INITIALIZER;
int items;

void *consumer(void *unused)
{
	pthread_mutex_lock(&m);
#ifdef BUG_IF
	if (items == 0)
#else
	while (items == 0)
#endif
		pthread_cond_wait(&c, &m);
	assert(items > 0);
	items--;
	pthread_mutex_unlock(&m);
	return NULL;
}

void *producer(void *unused)
{
	pthread_mutex_lock(&m);
	items++;
#ifndef BUG_NO_SIGNAL
	pthread_cond_signal(&c);
#endif
	pthread_mutex_unlock(&m);
	return NULL;
}

int main()
{
	pthread_t c1, c2, p1, p2;

	pthread_create(&c1, NULL, consumer, NULL);
	pthread_create(&c2, NULL, consumer, NULL);
	pthread_create(&p1, NULL, producer, NULL);
#ifndef BUG_ONE_PRODUCER
	pthread_create(&p2, NULL, producer, NULL);
#endif
	return 0;
}
