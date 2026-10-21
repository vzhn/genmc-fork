/*
 * GenMC -- Generic Model Checking.
 *
 * This project is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 *
 * Apache License 2.0:
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * MIT License:
 *     https://opensource.org/licenses/MIT
 */

#ifndef GENMC_SEXPR_OPS_HPP
#define GENMC_SEXPR_OPS_HPP

#include "genmc/Support/SVal.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>

/*
 * The arithmetic of the expression evaluator (SExprEvaluator), as functions
 * of the operands' values and width alone, so that it can be tested and
 * proved apart from the expression trees (tests/unit/SExprEvaluatorTest.cpp,
 * scripts/prove-sexpr-ops.sh).
 *
 * The semantics are LLVM's: values are kept zero-extended to the width of
 * their expression (as loaded values are), so results are wrapped to that
 * width and signed operations sign-extend their operands first. Where LLVM
 * gives no value (division by zero, signed division overflow, a shift by the
 * width or more) the result is 0: brute-force evaluation feeds any value to
 * an expression, so these must not trap. Expressions wider than an SVal are
 * evaluated at the SVal's width.
 */
namespace SExprOps {

/** The width an expression of width W is evaluated at */
inline auto evalWidth(unsigned w) -> unsigned { return std::min(w, SVal::width); }

/** V as a zero-extended value of width W */
inline auto wrap(SVal v, unsigned w) -> SVal { return w < SVal::width ? v.truncate(w) : v; }

/** The zero-extended V of width W, sign-extended */
inline auto sext(SVal v, unsigned w) -> SVal
{
	return w < SVal::width ? v.signExtendBottom(w) : v;
}

/* Casts */

inline auto zext(SVal v) -> SVal { return v; }
inline auto sextTo(SVal v, unsigned from, unsigned to) -> SVal { return wrap(sext(v, from), to); }
inline auto trunc(SVal v, unsigned to) -> SVal { return wrap(v, to); }

/* Logic: the result of a comparison or negation is a boolean (width 1) */

inline auto boolean(bool b) -> SVal { return SVal(b ? 1 : 0); }
inline auto lnot(SVal v) -> SVal { return boolean(!v.getBool()); }
inline auto select(SVal c, SVal t, SVal f) -> SVal { return c.getBool() ? t : f; }

/* Arithmetic on operands of width W */

inline auto add(SVal l, SVal r, unsigned w) -> SVal { return wrap(l + r, w); }
inline auto sub(SVal l, SVal r, unsigned w) -> SVal { return wrap(l - r, w); }
inline auto mul(SVal l, SVal r, unsigned w) -> SVal { return wrap(l * r, w); }

inline auto udiv(SVal l, SVal r) -> SVal { return r.get() == 0 ? SVal(0) : l / r; }
inline auto urem(SVal l, SVal r) -> SVal { return r.get() == 0 ? SVal(0) : l % r; }

inline auto sdiv(SVal l, SVal r, unsigned w) -> SVal
{
	auto ls = sext(l, w).getSigned();
	auto rs = sext(r, w).getSigned();
	/* The signed minimum of the width, divided by -1, overflows */
	auto ew = evalWidth(w);
	auto min = ew == SVal::width ? std::numeric_limits<int64_t>::min()
				     : -(static_cast<int64_t>(1) << (ew - 1));
	if (rs == 0 || (rs == -1 && ls == min))
		return SVal(0);
	return wrap(SVal(static_cast<uint64_t>(ls / rs)), w);
}

inline auto srem(SVal l, SVal r, unsigned w) -> SVal
{
	auto ls = sext(l, w).getSigned();
	auto rs = sext(r, w).getSigned();
	if (rs == 0 || rs == -1)
		return SVal(0);
	return wrap(SVal(static_cast<uint64_t>(ls % rs)), w);
}

inline auto band(SVal l, SVal r) -> SVal { return l & r; }
inline auto bor(SVal l, SVal r) -> SVal { return l | r; }
inline auto bxor(SVal l, SVal r) -> SVal { return l ^ r; }

inline auto shl(SVal l, SVal r, unsigned w) -> SVal
{
	return r.get() >= evalWidth(w) ? SVal(0) : wrap(l << r, w);
}

inline auto lshr(SVal l, SVal r, unsigned w) -> SVal
{
	return r.get() >= evalWidth(w) ? SVal(0) : l >> r;
}

inline auto ashr(SVal l, SVal r, unsigned w) -> SVal
{
	if (r.get() >= evalWidth(w))
		return SVal(0);
	return wrap(SVal(static_cast<uint64_t>(sext(l, w).getSigned() >> r.get())), w);
}

/* Comparisons of operands of width W */

inline auto eq(SVal l, SVal r) -> SVal { return boolean(l == r); }
inline auto ne(SVal l, SVal r) -> SVal { return boolean(l != r); }
inline auto ult(SVal l, SVal r) -> SVal { return boolean(l.ult(r)); }
inline auto ule(SVal l, SVal r) -> SVal { return boolean(l.ule(r)); }
inline auto ugt(SVal l, SVal r) -> SVal { return boolean(l.ugt(r)); }
inline auto uge(SVal l, SVal r) -> SVal { return boolean(l.uge(r)); }
inline auto slt(SVal l, SVal r, unsigned w) -> SVal { return boolean(sext(l, w).slt(sext(r, w))); }
inline auto sle(SVal l, SVal r, unsigned w) -> SVal { return boolean(sext(l, w).sle(sext(r, w))); }
inline auto sgt(SVal l, SVal r, unsigned w) -> SVal { return boolean(sext(l, w).sgt(sext(r, w))); }
inline auto sge(SVal l, SVal r, unsigned w) -> SVal { return boolean(sext(l, w).sge(sext(r, w))); }

} // namespace SExprOps

#endif /* GENMC_SEXPR_OPS_HPP */
