/*
 * The math.h operations that clang turns into floating-point intrinsics,
 * which GenMC lowers itself (IntrinsicLoweringPass): x * y + z
 * (llvm.fmuladd), fabs() (llvm.fabs), isnan() and the other classifiers
 * (llvm.is.fpclass), fmin() and fmax() (llvm.minnum, llvm.maxnum). Each
 * aborted GenMC with an LLVM error. Every class of value, in doubles and
 * floats; the operands are volatile, so that the compiler does not fold them.
 */
#include <assert.h>
#include <float.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>

volatile double one = 1.0, two = 2.0, quarter = 0.25, zero = 0.0, big = DBL_MAX;
volatile float fone = 1.0f, fzero = 0.0f;

void doubles(void)
{
	double nan = zero / zero, inf = big * two, sub = DBL_MIN * quarter, nzero = -zero;

	/* fmuladd */
	assert(one * two + quarter == 2.25);

	/* fabs */
	assert(fabs(-two) == 2.0 && fabs(two) == 2.0);
	assert(fabs(-inf) == inf && signbit(fabs(nzero)) == 0 && isnan(fabs(nan)));

	/* Classification */
	assert(isnan(nan) && !isnan(inf) && !isnan(one) && !isnan(zero));
	assert(isinf(inf) && isinf(-inf) && !isinf(nan) && !isinf(big));
	assert(isfinite(one) && isfinite(sub) && isfinite(zero) && !isfinite(inf) &&
	       !isfinite(nan));
	assert(isnormal(one) && isnormal(-big) && !isnormal(sub) && !isnormal(zero) &&
	       !isnormal(inf) && !isnormal(nan));
	assert(fpclassify(sub) == FP_SUBNORMAL && fpclassify(-sub) == FP_SUBNORMAL);
	assert(fpclassify(nzero) == FP_ZERO && fpclassify(zero) == FP_ZERO);
	assert(fpclassify(-one) == FP_NORMAL && fpclassify(-inf) == FP_INFINITE &&
	       fpclassify(nan) == FP_NAN);
	assert(signbit(nzero) && !signbit(zero) && signbit(-inf) && !signbit(sub));

	/* Each class on its own (the LangRef's mask bits, as clang's builtin
	 * passes them to llvm.is.fpclass) */
	volatile double snan = __builtin_nans("");
	double values[] = {snan, nan, -inf, -one, -sub, nzero, zero, sub, one, inf};
#define IN_CLASS(c) assert(!!__builtin_isfpclass(values[v], 1 << (c)) == (v == (c)))
	for (int v = 0; v < 10; v++) {
		IN_CLASS(0); IN_CLASS(1); IN_CLASS(2); IN_CLASS(3); IN_CLASS(4);
		IN_CLASS(5); IN_CLASS(6); IN_CLASS(7); IN_CLASS(8); IN_CLASS(9);
	}
	assert(__builtin_isfpclass(sub, 0x090) && !__builtin_isfpclass(one, 0x00f));

	/* fmin, fmax: a NaN loses */
	assert(fmin(one, two) == 1.0 && fmin(two, one) == 1.0 && fmin(-inf, one) == -inf);
	assert(fmax(one, two) == 2.0 && fmax(two, one) == 2.0 && fmax(inf, one) == inf);
	assert(fmin(nan, one) == 1.0 && fmin(one, nan) == 1.0 && isnan(fmin(nan, nan)));
	assert(fmax(nan, two) == 2.0 && fmax(two, nan) == 2.0 && isnan(fmax(nan, nan)));
}

void floats(void)
{
	float nan = fzero / fzero, inf = FLT_MAX * (fone + fone), sub = FLT_MIN / 4;

	assert(fone * 2.0f + 0.5f == 2.5f);
	assert(fabsf(-fone) == 1.0f);
	assert(isnan(nan) && isinf(-inf) && isfinite(sub) && !isnormal(sub) && isnormal(fone));
	assert(fpclassify(sub) == FP_SUBNORMAL && fpclassify(-fzero) == FP_ZERO);
	assert(fminf(nan, fone) == 1.0f && fmaxf(fone, -inf) == 1.0f);
}

void *thread(void *unused)
{
	doubles();
	floats();
	return NULL;
}

int main()
{
	pthread_t t;

	if (pthread_create(&t, NULL, thread, NULL))
		abort();
	doubles();
	floats();
	if (pthread_join(t, NULL))
		abort();
	return 0;
}
