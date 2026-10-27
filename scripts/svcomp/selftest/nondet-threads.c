/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* A race between the fourth thread and main, which only a nondet thread
 * count of 4 or more has; every thread reads a nondet value too, so that
 * there are more nondet calls than SVC_MAX_CHOICES (4). A run with more
 * nondet values must find the race: there the calls past the limit return
 * 0 instead of ending the execution */
#include <pthread.h>
#include <stdlib.h>

extern int __VERIFIER_nondet_int(void);
void assume_abort_if_not(int cond)
{
	if (!cond)
		abort();
}

int data[8];
int last;

void *worker(void *arg)
{
	long i = (long) arg;

	data[i] = __VERIFIER_nondet_int();
	if (i == 3)
		last = 1;
	return 0;
}

int main(void)
{
	pthread_t t[8];
	int n = __VERIFIER_nondet_int();

	assume_abort_if_not(1 <= n && n <= 8);
	for (long i = 0; i < n; i++)
		pthread_create(&t[i], 0, worker, (void *) i);
	int r = last;
	for (long i = 0; i < n; i++)
		pthread_join(t[i], 0);
	return r;
}
