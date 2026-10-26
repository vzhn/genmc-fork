/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* A thread reads another thread's ID once that thread has signalled, and
 * joins it. glibc stores the ID before the thread runs, so this is no race;
 * GenMC's pthread_create stored it after, and the read raced with the store */
#include <pthread.h>
#include <stdatomic.h>

pthread_t tids[1];
atomic_int started;

void *worker(void *arg)
{
	atomic_store_explicit(&started, 1, memory_order_release);
	return 0;
}

void *joiner(void *arg)
{
	while (!atomic_load_explicit(&started, memory_order_acquire))
		;
	pthread_join(tids[0], 0);
	return 0;
}

int main(void)
{
	pthread_t j;

	pthread_create(&j, 0, joiner, 0);
	pthread_create(&tids[0], 0, worker, 0);
	pthread_join(j, 0);
	return 0;
}
