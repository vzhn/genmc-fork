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

#include "CodeCondenserPass.hpp"
#include "passes/Transforms/BisimilarityCheckerPass.hpp"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/Casting.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include <algorithm>
#include <unordered_map>
#include <vector>

using namespace llvm;
using BsPoint = BisimilarityAnalysis::BisimilarityPoint;

/* Returns whether I belongs to the range [FROM, end of FROM's block) */
static auto isInRange(const Instruction *i, const Instruction *from) -> bool
{
	return i->getParent() == from->getParent() && !i->comesBefore(from);
}

/*
 * Returns whether the values defined in the range [FROM, end of block) are
 * used only within the range or by PHIs of the successors (as the value
 * incoming from the range's block).
 */
static auto isRangeUsedOnlyLocally(Instruction *from) -> bool
{
	auto *bb = from->getParent();
	for (auto it = from->getIterator(); it != bb->end(); ++it) {
		for (auto *user : it->users()) {
			auto *userInst = dyn_cast<Instruction>(user);
			if (!userInst)
				return false;
			if (isInRange(userInst, from))
				continue;
			auto *phi = dyn_cast<PHINode>(userInst);
			if (!phi)
				return false;
			for (auto k = 0U; k < phi->getNumIncomingValues(); k++)
				if (phi->getIncomingValue(k) == &*it && phi->getIncomingBlock(k) != bb)
					return false;
		}
	}
	return true;
}

/*
 * Returns whether the range [BBEG, end of block) computes the same values as
 * the range [ABEG, end of block) when it is executed in its place: each
 * operand must either be the same value (which then dominates both ranges)
 * or be the bisimilar counterpart of A's operand defined within B's range.
 * (A value defined earlier in B's block is undefined on the redirected paths.)
 */
static auto haveSameOperands(Instruction *aBeg, Instruction *bBeg,
			     const std::vector<BsPoint> &bsps) -> bool
{
	auto aIt = aBeg->getIterator();
	auto bIt = bBeg->getIterator();
	for (; aIt != aBeg->getParent()->end() && bIt != bBeg->getParent()->end(); ++aIt, ++bIt) {
		if (aIt->getNumOperands() != bIt->getNumOperands())
			return false;
		for (auto k = 0U; k < aIt->getNumOperands(); k++) {
			auto *opA = aIt->getOperand(k);
			auto *opB = bIt->getOperand(k);
			if (opA == opB)
				continue;
			auto *instA = dyn_cast<Instruction>(opA);
			auto *instB = dyn_cast<Instruction>(opB);
			if (!instA || !instB || !isInRange(instA, aBeg) || !isInRange(instB, bBeg) ||
			    !std::ranges::binary_search(bsps, BsPoint(instA, instB)))
				return false;
		}
	}
	return aIt == aBeg->getParent()->end() && bIt == bBeg->getParent()->end();
}

/*
 * Returns whether the range [ABEG, end of A's block) can be replaced by the
 * range [BBEG, end of B's block). Bisimilarity only ensures that the two ranges
 * perform the same operations and that they feed the same values to the PHIs
 * of the block that follows both; the replacement is still unsound if:
 *
 *   - a value of A's range is used elsewhere (the range is deleted),
 *   - a value of B's range is used elsewhere (the range is then also executed
 *     on the paths that used to execute A's, redefining the value there),
 *   - B's range uses a value that A's does not, defined earlier in B's block
 *     (undefined on those paths), or
 *   - a successor's PHI does not map A's incoming value to B's.
 */
static auto canCondense(Instruction *aBeg, Instruction *bBeg, const std::vector<BsPoint> &bsps)
	-> bool
{
	auto *aBB = aBeg->getParent();
	auto *bBB = bBeg->getParent();

	/* Splitting at a PHI, or redirecting edges into a block with PHIs, is not possible */
	if (isa<PHINode>(aBeg) || isa<PHINode>(bBeg))
		return false;

	if (!isRangeUsedOnlyLocally(aBeg) || !isRangeUsedOnlyLocally(bBeg) ||
	    !haveSameOperands(aBeg, bBeg, bsps))
		return false;

	for (auto *succ : successors(aBB)) {
		for (auto &phi : succ->phis()) {
			auto aIdx = phi.getBasicBlockIndex(aBB);
			auto bIdx = phi.getBasicBlockIndex(bBB);
			if (aIdx < 0 || bIdx < 0)
				return false;
			auto *valA = phi.getIncomingValue(static_cast<unsigned>(aIdx));
			auto *valB = phi.getIncomingValue(static_cast<unsigned>(bIdx));
			if (valA == valB)
				continue;
			auto *instA = dyn_cast<Instruction>(valA);
			auto *instB = dyn_cast<Instruction>(valB);
			if (!instA || !instB || !isInRange(instA, aBeg) || !isInRange(instB, bBeg) ||
			    !std::ranges::binary_search(bsps, BsPoint(instA, instB)))
				return false;
		}
	}
	return true;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto CodeCondenserPass::run(Function &F, FunctionAnalysisManager &FAM) -> PreservedAnalyses
{
	auto bsps = FAM.getResult<BisimilarityAnalysis>(F);

	/* Map blocks to bisimilarity points */
	std::unordered_map<BasicBlock *, std::vector<BsPoint>> bbsToBsps;
	for (const auto &bsp : bsps)
		bbsToBsps[bsp.first->getParent()].push_back(bsp);

	/* Condense the code: */
	for (auto &bbP : bbsToBsps) {
		/* find the first bisimilar point within each block (should not be
		 * TerminatorInst) at which condensing is sound... */
		for (auto &i : *bbP.first) {
			auto pIt = std::ranges::find_if(
				bbP.second, [&](const BsPoint &bsp) { return bsp.first == &i; });
			if (pIt == bbP.second.end())
				continue;
			if (pIt->first->isTerminator())
				continue;
			if (!canCondense(pIt->first, pIt->second, bsps))
				continue;

			/* ...and do the actual condensing */
			auto aIt = pIt->first->getIterator();
			auto bIt = pIt->second->getIterator();
			auto *aBB = aIt->getParent();
			auto *bBB = bIt->getParent();

			auto *aNewBB = (aBB->begin() != aIt || &*F.begin() == aBB)
					       ? aBB->splitBasicBlock(aIt)
					       : aBB;
			auto *bNewBB = (bBB->begin() != bIt || &*F.begin() == bBB)
					       ? bBB->splitBasicBlock(bIt)
					       : bBB;
			auto predIt = pred_begin(aNewBB);
			while (predIt != pred_end(aNewBB)) {
				auto tmpIt = predIt++;
				auto *pred = *tmpIt;
				auto *term = dyn_cast<BranchInst>(pred->getTerminator());
				if (!term)
					continue;

				for (auto i = 0U; i < term->getNumSuccessors(); i++)
					if (term->getSuccessor(i) == aNewBB)
						term->setSuccessor(i, bNewBB);
			}
			break;
		}
	}
	/* Clear unnecessary blocks */
	EliminateUnreachableBlocks(F);
	return PreservedAnalyses::none();
}
