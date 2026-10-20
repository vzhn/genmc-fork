#include <pthread.h>
#include <assert.h>

/* Waiting under the mutex, leaving the loop with a break */

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int ready, data;

void *producer(void *unused)
{
	pthread_mutex_lock(&m);
	data = 42;
	ready = 1;
	pthread_mutex_unlock(&m);
	return NULL;
}

void *consumer(void *unused)
{
	while (1) {
		pthread_mutex_lock(&m);
		if (ready) {
			assert(data == 42);
			pthread_mutex_unlock(&m);
			break;
		}
		pthread_mutex_unlock(&m);
	}
	return NULL;
}

int main()
{
	pthread_t p, c1, c2;

	pthread_create(&c1, NULL, consumer, NULL);
	pthread_create(&p, NULL, producer, NULL);
	pthread_create(&c2, NULL, consumer, NULL);
	pthread_join(p, NULL);
	pthread_join(c1, NULL);
	pthread_join(c2, NULL);
	return 0;
}
