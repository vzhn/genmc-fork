/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* Reads a malloc()ed block before writing it. SV-COMP does not count a read
 * of uninitialized memory as a memory-safety violation, so valid-memsafety
 * holds; GenMC reports such a read unless it runs with -svcomp */
#include <pthread.h>
#include <stdlib.h>

int *data;

void *worker(void *arg)
{
	int v = data[0];

	data[1] = v;
	return 0;
}

int main(void)
{
	pthread_t t;

	data = malloc(2 * sizeof(int));
	if (!data)
		return 0;
	pthread_create(&t, 0, worker, 0);
	pthread_join(t, 0);
	free(data);
	return 0;
}
