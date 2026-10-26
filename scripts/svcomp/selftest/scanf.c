/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* main reads x with scanf while the worker writes it: a race. GenMC's
 * stdio.h has no scanf; the shim stores a nondet value */
#include <pthread.h>
#include <stdio.h>

int x;

void *worker(void *arg)
{
	x = 1;
	return 0;
}

int main(void)
{
	pthread_t t;

	pthread_create(&t, 0, worker, 0);
	scanf("%d", &x);
	pthread_join(t, 0);
	return 0;
}
