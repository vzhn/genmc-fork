/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* The error needs a nondet value of 42, which the task compares with: the
 * shim's 0..2 miss it, a run with more nondet values must find it */
#include <pthread.h>
#include <assert.h>

extern int __VERIFIER_nondet_int(void);
void reach_error() { assert(0); }

int glob;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void *worker(void *arg)
{
	int t = __VERIFIER_nondet_int();

	pthread_mutex_lock(&m);
	if (t == 42)
		glob = 1;
	if (glob)
		reach_error();
	pthread_mutex_unlock(&m);
	return 0;
}

int main(void)
{
	pthread_t id;

	pthread_create(&id, 0, worker, 0);
	pthread_join(id, 0);
	return 0;
}
