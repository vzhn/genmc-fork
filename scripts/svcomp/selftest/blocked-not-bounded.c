/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* Executions abort for a reason of the task's own (the worker has not run
 * yet), with nondet values below the range's top: the bounds cut nothing,
 * so the clean result stands */
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <stdatomic.h>

extern _Bool __VERIFIER_nondet_bool(void);
void reach_error() { assert(0); }

atomic_int ready;

void *worker(void *arg)
{
	atomic_store(&ready, 1);
	return 0;
}

int main(void)
{
	pthread_t t;
	int b = __VERIFIER_nondet_bool();

	pthread_create(&t, 0, worker, 0);
	if (!atomic_load(&ready))
		abort();
	if (b > 1)
		reach_error();
	pthread_join(t, 0);
	return 0;
}
