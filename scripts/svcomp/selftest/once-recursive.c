/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* pthread_once with <pthread.h> and the shim's recursive mutexes (the task
 * mentions PTHREAD_MUTEX_RECURSIVE, so svcomp-run.py sets
 * SVC_RECURSIVE_MUTEX=1): the init routine sets up the mutex, which each
 * worker then takes twice */
#include <assert.h>
#include <pthread.h>

void reach_error() { assert(0); }

pthread_once_t once = PTHREAD_ONCE_INIT;
pthread_mutex_t m;
int count;

void init(void)
{
	pthread_mutexattr_t a;

	pthread_mutexattr_init(&a);
	pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m, &a);
	pthread_mutexattr_destroy(&a);
}

void *worker(void *arg)
{
	pthread_once(&once, init);
	pthread_mutex_lock(&m);
	pthread_mutex_lock(&m);
	count++;
	pthread_mutex_unlock(&m);
	pthread_mutex_unlock(&m);
	return 0;
}

int main(void)
{
	pthread_t t[2];

	for (int i = 0; i < 2; i++)
		pthread_create(&t[i], 0, worker, 0);
	for (int i = 0; i < 2; i++)
		pthread_join(t[i], 0);
	if (count != 2)
		reach_error();
	return 0;
}
