#include <pthread.h>

/* (SV-COMP pthread-race-challenges/thread-join-array-const-race) The last
 * spawn overwrites the first handle, so the first thread is never joined
 * and races with main's read */

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int data;

void *thread(void *unused)
{
	pthread_mutex_lock(&m);
	data = 1;
	pthread_mutex_unlock(&m);
	return NULL;
}

int main()
{
	pthread_t t[2];

	for (int i = 0; i < 2; i++)
		pthread_create(&t[i], NULL, thread, NULL);
	pthread_create(&t[0], NULL, thread, NULL);
	for (int i = 0; i < 2; i++)
		pthread_join(t[i], NULL);
	return data;
}
