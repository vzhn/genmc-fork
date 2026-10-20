#include <pthread.h>

/* The consumer waits under the mutex for a second item that is never
 * produced: each iteration takes and releases the mutex and sees nothing */

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int counter;

void *producer(void *unused)
{
	pthread_mutex_lock(&m);
	counter++;
	pthread_mutex_unlock(&m);
	return NULL;
}

void *consumer(void *unused)
{
	int i = 0;

	while (i < 2) {
		pthread_mutex_lock(&m);
		if (counter > 0) {
			counter--;
			i++;
		}
		pthread_mutex_unlock(&m);
	}
	return NULL;
}

int main()
{
	pthread_t p, c;

	pthread_create(&p, NULL, producer, NULL);
	pthread_create(&c, NULL, consumer, NULL);
	pthread_join(p, NULL);
	pthread_join(c, NULL);
	return 0;
}
