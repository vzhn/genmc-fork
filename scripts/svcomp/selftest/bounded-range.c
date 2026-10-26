/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* The task wants a value of at least 20, beyond the shim's nondet range
 * (0..2): every execution aborts, so a clean result would be a wrong "true" */
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>

extern unsigned int __VERIFIER_nondet_uint(void);
void reach_error() { assert(0); }

void assume_abort_if_not(int cond)
{
	if (!cond)
		abort();
}

int main(void)
{
	unsigned int n = __VERIFIER_nondet_uint();

	assume_abort_if_not(n >= 20);
	if (n == 25)
		reach_error();
	return 0;
}
