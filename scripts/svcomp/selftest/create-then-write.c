/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* main writes g right after creating the thread that writes it too: a race.
 * The write follows the THREAD_CREATE and stores the new thread's ID (2),
 * like pthread_create's own store, but it is the task's and races in
 * SV-COMP too, so it must not be labelled CREATE-RACE */
#include <pthread.h>

int g;

void *worker(void *arg)
{
	g = 1;
	return 0;
}

int main(void)
{
	pthread_t t;

	pthread_create(&t, 0, worker, 0);
	g = 2;
	return 0;
}
