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

#include "StrengthenCASPass.hpp"
#include "passes/Transforms/LoadAnnotationPass.hpp"

#include "genmc/Execution/LoadAnnotation.hpp"
#include "genmc/Support/Error.hpp"
#include "passes/InternalFunctions.hpp"
#include "passes/LLVMUtils.hpp"

#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DebugLoc.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>

#include <algorithm>
#include <format>
#include <string>
#include <type_traits>

using namespace llvm;

static auto sourceLocation(const Instruction *inst) -> std::string
{
	if (const auto &dl = inst->getDebugLoc())
		return std::format("{}:{}", dl->getFilename().str(), dl.getLine());
	return inst->getFunction()->getName().str();
}

static auto isSpinloopAssume(const Instruction &i) -> bool
{
	const auto *ci = dyn_cast<CallInst>(&i);
	if (!ci || !isAssumeFunction(getCalledFunOrStripValName(*ci)))
		return false;
	const auto *type = dyn_cast<ConstantInt>(ci->getArgOperand(1));
	return type && type->getZExtValue() == static_cast<std::underlying_type_t<AssumeType>>(
						       AssumeType::Spinloop);
}

/* Whether V is computed from a PHI of HEADER (without going through other PHIs) */
static auto dependsOnPHIOf(const Value *v, const BasicBlock *header,
			   SmallPtrSetImpl<const Value *> &visited) -> bool
{
	const auto *inst = dyn_cast<Instruction>(v);
	if (!inst || !visited.insert(inst).second)
		return false;
	if (isa<PHINode>(inst))
		return inst->getParent() == header;
	return std::ranges::any_of(inst->operands(), [&](const Use &op) {
		return dependsOnPHIOf(op.get(), header, visited);
	});
}

/*
 * Whether I is in a loop with a spin-end that depends on the loop's header PHIs. The
 * spin-assume transformation then prunes an iteration only if it leaves those PHIs
 * unchanged, so a spurious failure of a weak CAS in the loop is not necessarily pruned.
 * (Header PHIs that no spin-end uses, such as an unroll bound, do not matter.)
 */
static auto isInSpinloopWithState(const Instruction *i, const LoopInfo &LI) -> bool
{
	for (const auto *l = LI.getLoopFor(i->getParent()); l; l = l->getParentLoop()) {
		for (const auto *bb : l->blocks()) {
			if (LI.getLoopFor(bb) != l)
				continue;
			for (const auto &j : *bb) {
				SmallPtrSet<const Value *, 16> visited;
				if (isSpinloopAssume(j) &&
				    dependsOnPHIOf(cast<CallInst>(j).getArgOperand(0),
						   l->getHeader(), visited))
					return true;
			}
		}
	}
	return false;
}

auto StrengthenCASPass::run(Function &F, FunctionAnalysisManager &FAM) -> PreservedAnalyses
{
	const auto &lai = FAM.getResult<LoadAnnotationAnalysis>(F);
	const auto &LI = FAM.getResult<LoopAnalysis>(F);
	auto modified = false;

	for (auto &inst : instructions(F)) {
		auto *casi = dyn_cast<AtomicCmpXchgInst>(&inst);
		if (!casi || !casi->isWeak())
			continue;

		/* If the weak CAS is annotated, its spurious failures are pruned anyway, so
		 * make it strong (not if the spin-end depends on header PHIs: an iteration
		 * that changes them is not pruned) */
		auto it = lai.annotMap.find(casi);
		if (it != lai.annotMap.end() && it->second.first == AssumeType::Spinloop &&
		    !isInSpinloopWithState(casi, LI)) {
			casi->setWeak(false);
			modified = true;
			continue;
		}

		/* NB: keep this message free of the words the test driver greps for in
		 * GenMC's output ("explored", "blocked", "wall-clock"). */
		WARN("Weak CAS at {} could not be strengthened to a strong "
		     "one. Strong CASes enable more effective verification.",
		     sourceLocation(casi));
	}
	return modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
