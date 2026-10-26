/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/* The error needs five nondet values, one more than the shim's default
 * --max-choices: every execution that could reach it is cut, so a clean
 * result would be a wrong "true" */
#include <pthread.h>
#include <assert.h>

extern int __VERIFIER_nondet_int(void);
void reach_error() { assert(0); }

int main(void)
{
	int n = 0;

	for (int i = 0; i < 5; i++)
		n += __VERIFIER_nondet_int() > 0;
	if (n == 5)
		reach_error();
	return 0;
}
