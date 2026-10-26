/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* SV-COMP's malloc may return NULL: v[0] then dereferences NULL */
#include <pthread.h>
#include <stdlib.h>

char *v;

int main(void)
{
	v = malloc(8);
	v[0] = 'B';
	free(v);
	return 0;
}
