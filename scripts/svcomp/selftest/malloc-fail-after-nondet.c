/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* main reads a nondet index, then aborts if malloc fails: that abort is
 * the task's NULL check, not a value out of range, so the bounds check
 * must leave the clean result alone */
#include <pthread.h>
#include <stdlib.h>

extern int __VERIFIER_nondet_int(void);

void assume_abort_if_not(int cond)
{
	if (!cond)
		abort();
}

int main(void)
{
	int i = __VERIFIER_nondet_int();

	assume_abort_if_not(0 <= i && i < 10);
	int *p = malloc(10 * sizeof(int));

	if (!p)
		abort();
	p[i] = 1;
	free(p);
	return 0;
}
