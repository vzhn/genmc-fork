/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* The source input-file.i was made from. Like many SV-COMP tasks, it needs a
 * header the task does not ship, so only the .i (its input_files) builds */
#include <pthread.h>
#include "input-file-missing.h"

int x;

void *worker(void *arg)
{
	__atomic_store_n(&x, __atomic_load_n(&x, 5) + 1, 5);
	return 0;
}

int main(void)
{
	pthread_t t;

	pthread_create(&t, 0, worker, 0);
	__atomic_store_n(&x, __atomic_load_n(&x, 5) + 1, 5);
	pthread_join(t, 0);
	if (__atomic_load_n(&x, 5) != 2)
		reach_error();
	return 0;
}
