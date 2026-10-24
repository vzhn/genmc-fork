/*
 * The interpreter's floating-point operations: arithmetic and negation,
 * every ordered and unordered comparison (with a NaN), and the conversions
 * between integers, floats and doubles. The operands are volatile, so that
 * the compiler does not fold them; the values are exact in binary. A
 * thread computes them too, from a value it reads. (fmod() and the other
 * libm functions are external functions GenMC does not run.)
 */
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>

volatile double a = 1.5, b = 0.25, zero = 0.0;
volatile float f = 2.5f;
volatile int i = -7;
volatile unsigned u = 3000000000u;

atomic_int scale;

void check(double s)
{
	volatile double nan = zero / zero;

	/* Arithmetic */
	assert(a + b == 1.75 * s);
	assert(a - b == 1.25 * s);
	assert(a * b == 0.375 * s * s);
	assert(a / b == 6.0);
	assert(-a == -1.5 * s);

	/* Ordered comparisons */
	assert(a > b && a >= b && b < a && b <= a && a != b && !(a == b));
	assert(a >= a && a <= a);

	/* Unordered: every ordered comparison with a NaN is false */
	assert(nan != nan);
	assert(!(nan == nan) && !(nan < a) && !(nan > a) && !(nan <= a) && !(nan >= a));
	assert(isunordered(nan, a) && !isunordered(a, b));
	assert(!isless(nan, a) && !isgreater(nan, a) && !islessequal(nan, a) &&
	       !isgreaterequal(nan, a) && !islessgreater(nan, a) && islessgreater(a, b));
	assert(isnan(nan) && !isnan(a));

	/* Conversions */
	assert((int)(a * 3) == 4 * (int)s);   /* fptosi (truncates) */
	assert((int)-a == -1);                /* toward zero */
	assert((unsigned)(a * 2e9) == 3000000000u * (unsigned)s); /* fptoui */
	assert((double)i == -7.0);            /* sitofp */
	assert((double)u == 3e9);             /* uitofp */
	assert((float)(a * s) == 1.5f * (float)s);  /* fptrunc */
	assert((double)f == 2.5);             /* fpext */
	assert((float)f / 2 == 1.25f);        /* float arithmetic */
}

void *thread(void *unused)
{
	check(atomic_load_explicit(&scale, memory_order_relaxed));
	return NULL;
}

int main()
{
	pthread_t t;

	atomic_store_explicit(&scale, 1, memory_order_relaxed);
	if (pthread_create(&t, NULL, thread, NULL))
		abort();
	check(1.0);
	if (pthread_join(t, NULL))
		abort();
	return 0;
}
