#include <pthread.h>
#include <assert.h>

/* The mutex is released before the loop and at the end of each iteration;
 * the compiler merges the two unlocks into the loop header, so each
 * iteration starts by releasing the mutex and taking it again */

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int c, seen;

void *waiter(void *unused)
{
	pthread_mutex_lock(&m);
	seen = 1;
	pthread_mutex_unlock(&m);
	while (1) {
		pthread_mutex_lock(&m);
		if (c >= 1) {
			c--;
			pthread_mutex_unlock(&m);
			break;
		}
		pthread_mutex_unlock(&m);
	}
	return NULL;
}

void *setter(void *unused)
{
	pthread_mutex_lock(&m);
	c = 1;
	pthread_mutex_unlock(&m);
	return NULL;
}

int main()
{
	pthread_t w, s;

	pthread_create(&w, NULL, waiter, NULL);
	pthread_create(&s, NULL, setter, NULL);
	pthread_join(w, NULL);
	pthread_join(s, NULL);
	assert(c == 0 && seen);
	return 0;
}
