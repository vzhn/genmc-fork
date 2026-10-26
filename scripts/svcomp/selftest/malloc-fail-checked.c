/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* The task checks malloc's result, passing the pointer as an int as
 * SV-COMP's tasks do (gcc only warns about that); memory safety holds */
#include <pthread.h>
#include <stdlib.h>

void assume_abort_if_not(int cond)
{
	if (!cond)
		abort();
}

int main(void)
{
	char *v = malloc(8);

	assume_abort_if_not(v);
	v[0] = 'B';
	free(v);
	return 0;
}
