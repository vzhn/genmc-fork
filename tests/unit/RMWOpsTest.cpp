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
 * executeRMWBinOp() against LLVM's atomicrmw (the value it stores, computed
 * with llvm::APInt at the access width). Operands are zero-extended to the
 * access width, as GenMC keeps values, and so must the result be (#54).
 * Exhaustive at 1 byte, boundary values at 2, 4 and 8 bytes.
 */

#include <algorithm>
#include <cstdint>
#include <functional>
#include <vector>

#include <gtest/gtest.h>
#include <llvm/ADT/APInt.h>

#include "genmc/Support/RMWOps.hpp"

namespace {

using llvm::APInt;

auto stored(RMWBinOp op, const APInt &a, const APInt &b) -> APInt
{
	switch (op) {
	case RMWBinOp::Xchg:
		return b;
	case RMWBinOp::Add:
		return a + b;
	case RMWBinOp::Sub:
		return a - b;
	case RMWBinOp::And:
		return a & b;
	case RMWBinOp::Nand:
		return ~(a & b);
	case RMWBinOp::Or:
		return a | b;
	case RMWBinOp::Xor:
		return a ^ b;
	case RMWBinOp::Max:
		return a.sgt(b) ? a : b;
	case RMWBinOp::Min:
		return a.slt(b) ? a : b;
	case RMWBinOp::UMax:
		return a.ugt(b) ? a : b;
	case RMWBinOp::UMin:
		return a.ult(b) ? a : b;
	}
	return b;
}

auto values(unsigned w) -> std::vector<uint64_t>
{
	if (w == 8) {
		std::vector<uint64_t> vs(256);
		for (auto i = 0U; i < 256; i++)
			vs[i] = i;
		return vs;
	}
	auto mask = w == 64 ? ~uint64_t(0) : (uint64_t(1) << w) - 1;
	auto smin = uint64_t(1) << (w - 1);
	std::vector<uint64_t> vs = {0,
				    1,
				    2,
				    mask,
				    mask - 1,
				    smin,
				    smin - 1,
				    smin + 1,
				    0x5555555555555555ULL,
				    0xaaaaaaaaaaaaaaaaULL};
	for (auto &v : vs)
		v &= mask;
	std::ranges::sort(vs);
	vs.erase(std::unique(vs.begin(), vs.end()), vs.end());
	return vs;
}

} // namespace

TEST(RMWOpsTest, MatchesAtomicRMW)
{
	for (auto bytes : {1U, 2U, 4U, 8U}) {
		auto w = bytes * 8;
		auto vs = values(w);
		for (auto i = 0U; i <= static_cast<unsigned>(RMWBinOp::LAST); i++) {
			auto op = static_cast<RMWBinOp>(i);
			auto mismatches = 0;
			for (auto a : vs) {
				for (auto b : vs) {
					auto got =
						executeRMWBinOp(SVal(a), SVal(b), ASize(bytes), op)
							.get();
					auto want =
						stored(op, APInt(w, a), APInt(w, b)).getZExtValue();
					if (got != want && ++mismatches <= 3)
						ADD_FAILURE()
							<< "op " << i << " at " << w
							<< " bits: " << a << ", " << b << " gives "
							<< got << ", LLVM " << want;
				}
			}
			EXPECT_EQ(mismatches, 0) << "op " << i << " at " << w << " bits";
		}
	}
}
