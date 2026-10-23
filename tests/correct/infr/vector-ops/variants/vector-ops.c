/*
 * The interpreter's vector operations: insertelement and extractelement
 * at a variable index, shufflevector (which crashed: the interpreter read
 * the mask as an operand, which it has not been since LLVM 11),
 * element-wise arithmetic and comparisons, on integers and doubles. The
 * operands are volatile, so that the compiler does not fold them.
 */
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

typedef int v4i __attribute__((vector_size(16)));
typedef double v2d __attribute__((vector_size(16)));
typedef float v4f __attribute__((vector_size(16)));

volatile int k = 2;
volatile int base = 1;

void check(void)
{
	v4i a = {base, base + 1, base + 2, base + 3}, b = {5, 6, 7, 8};

	/* insertelement, extractelement */
	a[k] = 9;
	assert(a[k] == 9 && a[0] == 1 && a[3] == 4);

	/* shufflevector: from both sources, a repeated element, a lane left undefined */
	v4i s = __builtin_shufflevector(a, b, 7, 0, 5, 2);
	assert(s[0] == 8 && s[1] == 1 && s[2] == 6 && s[3] == 9);
	v4i r = __builtin_shufflevector(a, a, 3, 3, -1, 0);
	assert(r[0] == 4 && r[1] == 4 && r[3] == 1);
	v2d d = {base * 1.5, base * -2.0};
	v2d swapped = __builtin_shufflevector(d, d, 1, 0);
	assert(swapped[0] == -2.0 && swapped[1] == 1.5);
	v4f f = {base * 1.0f, 2.0f, 3.0f, 4.0f};
	v4f g = __builtin_shufflevector(f, f, 2, 3, 0, 1);
	assert(g[0] == 3.0f && g[3] == 2.0f);

	/* Element-wise arithmetic and comparisons */
	v4i c = a + b, m = a * b;
	assert(c[0] == 6 && c[3] == 12 && m[2] == 63);
	v4i lt = a < b;
	assert(lt[0] == -1 && lt[2] == 0);
	v2d e = d * d;
	assert(e[0] == 2.25 && e[1] == 4.0);
}

void *thread(void *unused)
{
	check();
	return NULL;
}

int main()
{
	pthread_t t;

	if (pthread_create(&t, NULL, thread, NULL))
		abort();
	check();
	if (pthread_join(t, NULL))
		abort();
	return 0;
}
