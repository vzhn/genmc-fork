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

#include "LoadAnnotationPass.hpp"
#include "genmc/ADT/VSet.hpp"
#include "genmc/Execution/LoadAnnotation.hpp"
#include "genmc/Support/Error.hpp"
#include "passes/InternalFunctions.hpp"
#include "passes/LLVMUtils.hpp"
#include "passes/Transforms/InstAnnotator.hpp"

#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/Casting.h>

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

using namespace llvm;

/* Helper for getSourceLoads() -- see below */
static void calcSourceLoads(Instruction *i, VSet<PHINode *> phis,
			    std::vector<Instruction *> &source)
{
	if (!i)
		return;

	/* Don't go past stores or allocas (CASes are OK) */
	if (isa<StoreInst>(i) || isa<AtomicRMWInst>(i) || isa<AllocaInst>(i))
		return;

	/* If we reached a (source) load, collect it */
	if (isa<LoadInst>(i) || isa<AtomicCmpXchgInst>(i)) {
		source.push_back(i);
		return;
	}

	/* If this is an already encountered Φ, don't go into circles */
	if (auto *phi = dyn_cast<PHINode>(i)) {
		if (phis.contains(phi))
			return;
		phis.insert(phi);
	}

	/* Otherwise, recurse */
	for (auto &use : i->operands()) {
		if (auto *pi = dyn_cast<Instruction>(use.get())) {
			calcSourceLoads(pi, phis, source);
		} else if (isa<Constant>(use.get())) {
			if (auto *phi = dyn_cast<PHINode>(i)) {
				auto *term = phi->getIncomingBlock(use)->getTerminator();
				if (auto *bi = dyn_cast<BranchInst>(term))
					if (bi->isConditional())
						calcSourceLoads(
							dyn_cast<Instruction>(bi->getCondition()),
							phis, source);
			}
		}
	}
}

/*
 * Returns the source loads of an assume statement, that is,
 * loads the result of which is used in the assume.
 */
static auto getSourceLoads(CallInst *assm) -> std::vector<Instruction *>
{
	const VSet<PHINode *> phis;
	std::vector<Instruction *> source;

	if (auto *arg = dyn_cast<Instruction>(assm->getOperand(0)))
		calcSourceLoads(arg, phis, source);
	std::ranges::sort(source);
	source.erase(std::ranges::unique(source).begin(), source.end());
	return source;
}

/* Whether I may lie between LI and an assume on a path along which LI is annotatable */
static auto isClearFor(const Instruction &i, const Instruction *li) -> bool
{
	return &i == li || (!hasSideEffects(&i) /* also CASes */ && !isa<LoadInst>(&i));
}

static auto isClearRangeFor(BasicBlock::const_iterator begin, BasicBlock::const_iterator end,
			    const Instruction *li) -> bool
{
	return std::all_of(begin, end, [&](const Instruction &i) { return isClearFor(i, li); });
}

/*
 * Whether some path from LI to ASSM is clear, i.e., has neither side effects
 * nor other loads on it.  Only the part of LI's block after LI and the part of
 * ASSM's block before ASSM count, and blocks strictly inside the path must be
 * clear as a whole.  The path is simple and stops at the first visit of LI's
 * block, so finding one is a backward reachability search from ASSM's block
 * through clear blocks.  (Enumerating the paths is exponential in the size of
 * the CFG and hangs on large inlined functions.)
 */
static auto hasClearPath(const Instruction *li, const CallInst *assm) -> bool
{
	const auto *loadBB = li->getParent();
	const auto *assmBB = assm->getParent();

	/* Same block: the only path is the one inside the block */
	if (loadBB == assmBB)
		return li->comesBefore(assm) &&
		       isClearRangeFor(std::next(li->getIterator()), assm->getIterator(), li);

	if (!isClearRangeFor(assmBB->begin(), assm->getIterator(), li) ||
	    !isClearRangeFor(std::next(li->getIterator()), loadBB->end(), li))
		return false;

	SmallPtrSet<const BasicBlock *, 16> visited = {assmBB};
	SmallVector<const BasicBlock *, 16> worklist = {assmBB};
	while (!worklist.empty()) {
		const auto *bb = worklist.pop_back_val();
		for (const auto *pred : predecessors(bb)) {
			if (pred == loadBB)
				return true;
			if (visited.insert(pred).second &&
			    isClearRangeFor(pred->begin(), pred->end(), li))
				worklist.push_back(pred);
		}
	}
	return false;
}

/*
 * Given an assume's source loads, returns the annotatable ones.
 */
static auto filterAnnotatableFromSource(CallInst *assm, const std::vector<Instruction *> &source)
	-> std::vector<Instruction *>
{
	std::vector<Instruction *> result;

	/* Collect candidates for which the path to the assume is clear */
	for (auto *li : source)
		if (hasClearPath(li, assm))
			result.push_back(li);
	return result;
}

/*
 * Returns all of ASSM's annotatable loads
 */
static auto getAnnotatableLoads(CallInst *assm) -> std::vector<Instruction *>
{
	if (!isAssumeFunction(getCalledFunOrStripValName(*assm)))
		return {}; /* yet another check... */

	auto sourceLoads = getSourceLoads(assm);
	return filterAnnotatableFromSource(assm, sourceLoads);
}

static auto extractAssumeArgument(CallInst *assume) -> uint64_t
{
	auto *arg = dyn_cast<Constant>(assume->getArgOperand(1));
	VERIFY(arg && arg->getType()->isIntegerTy());

	return arg->getUniqueInteger().getLimitedValue();
}

auto LoadAnnotationAnalysis::run(Function &F, FunctionAnalysisManager & /*FAM*/) -> Result
{
	InstAnnotator annotator;

	for (auto &i : instructions(F)) {
		auto *call = llvm::dyn_cast<llvm::CallInst>(&i);
		if (call && isAssumeFunction(getCalledFunOrStripValName(*call))) {
			auto loads = getAnnotatableLoads(call);
			for (auto *l : loads) {
				auto rawType = extractAssumeArgument(call);
				VERIFY(rawType <= static_cast<uint64_t>(AssumeType::Await));
				auto type = static_cast<AssumeType>(rawType);
				if (auto annot = annotator.annotate(l))
					result_.annotMap[l] = std::make_pair(type, std::move(annot));
			}
		}
	}
	return result_;
}

auto LoadAnnotationPass::run(Function &F, FunctionAnalysisManager &FAM) -> PreservedAnalyses
{
	LAI_ = FAM.getResult<LoadAnnotationAnalysis>(F);
	return PreservedAnalyses::all();
}
