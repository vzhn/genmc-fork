/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* pthread_once with <pthread.h>: two once objects run the init routine
 * twice, so the error is reachable */
#include <assert.h>
#include <pthread.h>

void reach_error() { assert(0); }

pthread_once_t once[2] = { PTHREAD_ONCE_INIT, PTHREAD_ONCE_INIT };
int inits;

void init(void)
{
	__atomic_fetch_add(&inits, 1, __ATOMIC_RELAXED);
}

void *worker(void *arg)
{
	pthread_once(&once[(long) arg], init);
	return 0;
}

int main(void)
{
	pthread_t t[2];

	for (long i = 0; i < 2; i++)
		pthread_create(&t[i], 0, worker, (void *) i);
	for (int i = 0; i < 2; i++)
		pthread_join(t[i], 0);
	if (__atomic_load_n(&inits, __ATOMIC_RELAXED) == 2)
		reach_error();
	return 0;
}
