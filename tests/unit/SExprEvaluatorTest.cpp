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

/*
 * SExprEvaluator against LLVM's integer semantics (llvm::APInt), which is
 * what the expressions mean: they are built from LLVM instructions (the
 * annotations of await, spin-loop and assume conditions). Values are
 * zero-extended to their expression's width; where LLVM gives no value
 * (division by zero, signed division overflow, a shift by the width or
 * more), GenMC's evaluator gives 0.
 *
 * Width 8 is checked exhaustively (all operand pairs), the other widths on
 * boundary values: the evaluator's width handling does not depend on the
 * width, so a mistake in it shows at 8 bits (as #54, #56 and #64 would have).
 */

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>
#include <llvm/ADT/APInt.h>

#include "genmc/Support/SExpr.hpp"
#include "genmc/Support/SExprVisitor.hpp"

namespace {

using Expr = SExpr<int>;
using ExprP = std::unique_ptr<Expr>;
using llvm::APInt;

auto cst(unsigned w, uint64_t v) -> ExprP { return ConcreteExpr<int>::create(w, SVal(v)); }

auto evaluate(const ExprP &e) -> uint64_t
{
	return SExprEvaluator<int>().evaluate(e.get(), SVal(0)).get();
}

auto ap(unsigned w, uint64_t v) -> APInt { return {w, v}; }

/* An operator: GenMC's expression, and LLVM's value (nullopt: no value, 0 in GenMC) */
struct BinOp {
	const char *name;
	std::function<ExprP(ExprP, ExprP)> build;
	std::function<std::optional<APInt>(const APInt &, const APInt &)> llvm;
};

auto zeroIfNone(std::optional<APInt> v) -> uint64_t
{
	return v ? v->getZExtValue() : 0;
}

auto boolean(bool b) -> std::optional<APInt> { return APInt(1, b ? 1 : 0); }

auto binOps() -> const std::vector<BinOp> &
{
	static const std::vector<BinOp> ops = {
		{"add",
		 [](ExprP l, ExprP r) -> ExprP {
			 return AddExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) -> std::optional<APInt> { return a + b; }},
		{"sub",
		 [](ExprP l, ExprP r) -> ExprP {
			 return SubExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) -> std::optional<APInt> { return a - b; }},
		{"mul",
		 [](ExprP l, ExprP r) -> ExprP {
			 return MulExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) -> std::optional<APInt> { return a * b; }},
		{"udiv",
		 [](ExprP l, ExprP r) -> ExprP {
			 return UDivExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) -> std::optional<APInt> {
			 if (b.isZero())
				 return std::nullopt;
			 return a.udiv(b);
		 }},
		{"sdiv",
		 [](ExprP l, ExprP r) -> ExprP {
			 return SDivExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) -> std::optional<APInt> {
			 if (b.isZero() || (a.isMinSignedValue() && b.isAllOnes()))
				 return std::nullopt;
			 return a.sdiv(b);
		 }},
		{"urem",
		 [](ExprP l, ExprP r) -> ExprP {
			 return URemExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) -> std::optional<APInt> {
			 if (b.isZero())
				 return std::nullopt;
			 return a.urem(b);
		 }},
		{"srem",
		 [](ExprP l, ExprP r) -> ExprP {
			 return SRemExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) -> std::optional<APInt> {
			 if (b.isZero() || (a.isMinSignedValue() && b.isAllOnes()))
				 return std::nullopt;
			 return a.srem(b);
		 }},
		{"and",
		 [](ExprP l, ExprP r) -> ExprP {
			 return AndExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) -> std::optional<APInt> { return a & b; }},
		{"or",
		 [](ExprP l, ExprP r) -> ExprP {
			 return OrExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) -> std::optional<APInt> { return a | b; }},
		{"xor",
		 [](ExprP l, ExprP r) -> ExprP {
			 return XorExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) -> std::optional<APInt> { return a ^ b; }},
		{"shl",
		 [](ExprP l, ExprP r) -> ExprP {
			 return ShlExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) -> std::optional<APInt> {
			 if (b.uge(a.getBitWidth()))
				 return std::nullopt;
			 return a.shl(b);
		 }},
		{"lshr",
		 [](ExprP l, ExprP r) -> ExprP {
			 return LShrExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) -> std::optional<APInt> {
			 if (b.uge(a.getBitWidth()))
				 return std::nullopt;
			 return a.lshr(b);
		 }},
		{"ashr",
		 [](ExprP l, ExprP r) -> ExprP {
			 return AShrExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) -> std::optional<APInt> {
			 if (b.uge(a.getBitWidth()))
				 return std::nullopt;
			 return a.ashr(b);
		 }},
		{"eq",
		 [](ExprP l, ExprP r) -> ExprP {
			 return EqExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) { return boolean(a.eq(b)); }},
		{"ne",
		 [](ExprP l, ExprP r) -> ExprP {
			 return NeExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) { return boolean(a.ne(b)); }},
		{"ult",
		 [](ExprP l, ExprP r) -> ExprP {
			 return UltExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) { return boolean(a.ult(b)); }},
		{"ule",
		 [](ExprP l, ExprP r) -> ExprP {
			 return UleExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) { return boolean(a.ule(b)); }},
		{"ugt",
		 [](ExprP l, ExprP r) -> ExprP {
			 return UgtExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) { return boolean(a.ugt(b)); }},
		{"uge",
		 [](ExprP l, ExprP r) -> ExprP {
			 return UgeExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) { return boolean(a.uge(b)); }},
		{"slt",
		 [](ExprP l, ExprP r) -> ExprP {
			 return SltExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) { return boolean(a.slt(b)); }},
		{"sle",
		 [](ExprP l, ExprP r) -> ExprP {
			 return SleExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) { return boolean(a.sle(b)); }},
		{"sgt",
		 [](ExprP l, ExprP r) -> ExprP {
			 return SgtExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) { return boolean(a.sgt(b)); }},
		{"sge",
		 [](ExprP l, ExprP r) -> ExprP {
			 return SgeExpr<int>::create(std::move(l), std::move(r));
		 },
		 [](const APInt &a, const APInt &b) { return boolean(a.sge(b)); }},
	};
	return ops;
}

/* Values that stress width handling at width W: around 0, the signed and
 * unsigned extremes, and alternating bits */
auto boundaryValues(unsigned w) -> std::vector<uint64_t>
{
	auto mask = w == 64 ? ~uint64_t(0) : (uint64_t(1) << w) - 1;
	auto smin = uint64_t(1) << (w - 1);
	std::vector<uint64_t> vs = {0,
				    1,
				    2,
				    3,
				    mask,
				    mask - 1,
				    smin,
				    smin - 1,
				    smin + 1,
				    0x5555555555555555ULL,
				    0xaaaaaaaaaaaaaaaaULL,
				    w - 1,
				    w,
				    w + 1};
	for (auto &v : vs)
		v &= mask;
	std::ranges::sort(vs);
	vs.erase(std::unique(vs.begin(), vs.end()), vs.end());
	return vs;
}

/* Checks OP at width W on all pairs of VALUES; reports the first mismatches */
void checkBinOp(const BinOp &op, unsigned w, const std::vector<uint64_t> &values)
{
	auto mismatches = 0;
	for (auto a : values) {
		for (auto b : values) {
			auto got = evaluate(op.build(cst(w, a), cst(w, b)));
			auto want = zeroIfNone(op.llvm(ap(w, a), ap(w, b)));
			if (got != want && ++mismatches <= 3)
				ADD_FAILURE() << op.name << " at width " << w << ": " << a << ", "
					      << b << " gives " << got << ", LLVM " << want;
		}
	}
	EXPECT_EQ(mismatches, 0) << op.name << " at width " << w;
}

auto allValues(unsigned w) -> std::vector<uint64_t>
{
	std::vector<uint64_t> vs(uint64_t(1) << w);
	for (auto i = 0U; i < vs.size(); i++)
		vs[i] = i;
	return vs;
}

} // namespace

TEST(SExprEvaluatorTest, BinaryOpsExhaustiveAt8Bits)
{
	for (const auto &op : binOps())
		checkBinOp(op, 8, allValues(8));
}

TEST(SExprEvaluatorTest, BinaryOpsAtBoundaries)
{
	for (auto w : {1U, 16U, 32U, 64U})
		for (const auto &op : binOps())
			checkBinOp(op, w, boundaryValues(w));
}

TEST(SExprEvaluatorTest, Casts)
{
	auto mismatches = 0;
	auto check = [&](const char *name, unsigned from, unsigned to, uint64_t a, uint64_t got,
			 const APInt &want) {
		if (got != want.getZExtValue() && ++mismatches <= 5)
			ADD_FAILURE() << name << " i" << from << " " << a << " to i" << to
				      << " gives " << got << ", LLVM " << want.getZExtValue();
	};
	const std::vector<unsigned> widths = {1, 8, 16, 32, 64};
	for (auto from : widths) {
		auto values = from == 8 ? allValues(8) : boundaryValues(from);
		for (auto to : widths) {
			for (auto a : values) {
				if (to > from) {
					check("zext", from, to, a,
					      evaluate(ZExtExpr<int>::create(to, cst(from, a))),
					      ap(from, a).zext(to));
					check("sext", from, to, a,
					      evaluate(SExtExpr<int>::create(to, cst(from, a))),
					      ap(from, a).sext(to));
				} else if (to < from) {
					check("trunc", from, to, a,
					      evaluate(TruncExpr<int>::create(to, cst(from, a))),
					      ap(from, a).trunc(to));
				}
			}
		}
	}
	EXPECT_EQ(mismatches, 0);
}

/* A logical negation: 1 iff its operand is 0 (#64: visitNotExpr() negated
 * the operand's pointer, so every negation was 0) */
TEST(SExprEvaluatorTest, Not)
{
	for (auto w : {1U, 8U, 32U})
		for (auto a : boundaryValues(w))
			EXPECT_EQ(evaluate(NotExpr<int>::create(cst(w, a))), a == 0 ? 1U : 0U)
				<< "not i" << w << " " << a;
}

TEST(SExprEvaluatorTest, SelectAndLogic)
{
	for (auto c : {0U, 1U})
		EXPECT_EQ(evaluate(SelectExpr<int>::create(cst(1, c), cst(32, 7), cst(32, 9))),
			  c ? 7U : 9U);
	for (auto a : {0U, 1U})
		for (auto b : {0U, 1U}) {
			EXPECT_EQ(evaluate(ConjunctionExpr<int>::create(cst(1, a), cst(1, b))),
				  a && b ? 1U : 0U);
			EXPECT_EQ(evaluate(DisjunctionExpr<int>::create(cst(1, a), cst(1, b))),
				  a || b ? 1U : 0U);
		}
}

/* A register gets the value the brute-force evaluation substitutes */
TEST(SExprEvaluatorTest, Register)
{
	auto e = AddExpr<int>::create(RegisterExpr<int>::create(8, 0), cst(8, 100));
	EXPECT_EQ(SExprEvaluator<int>().evaluate(e.get(), SVal(200)).get(), 44U);
}
