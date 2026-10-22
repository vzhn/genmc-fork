/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* pthread_once with <pthread.h>: the init routine runs once, and every
 * caller returns only after it is done and sees its writes, so neither
 * check fails and nothing races */
#include <assert.h>
#include <pthread.h>

void reach_error() { assert(0); }

pthread_once_t once = PTHREAD_ONCE_INIT;
int inits, data;

void init(void)
{
	inits++;
	data = 42;
}

void *worker(void *arg)
{
	pthread_once(&once, init);
	if (data != 42)
		reach_error();
	return 0;
}

int main(void)
{
	pthread_t t[2];

	for (int i = 0; i < 2; i++)
		pthread_create(&t[i], 0, worker, 0);
	pthread_once(&once, init);
	for (int i = 0; i < 2; i++)
		pthread_join(t[i], 0);
	if (inits != 1)
		reach_error();
	return 0;
}
