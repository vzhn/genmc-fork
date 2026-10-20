#include <pthread.h>
#include <assert.h>

/* A consumer waits under the mutex, counting what it took in a loop
 * variable: an iteration that takes and releases the mutex but takes
 * nothing repeats the previous one */

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int counter;

void *producer(void *unused)
{
	for (int i = 0; i < 2; i++) {
		pthread_mutex_lock(&m);
		counter++;
		pthread_mutex_unlock(&m);
	}
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
	assert(counter == 0);
	return 0;
}
