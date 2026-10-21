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

#include "FunctionInlinerPass.hpp"
#include "passes/InternalFunctions.hpp"

#include <llvm/ADT/SCCIterator.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Casting.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <algorithm>
#include <vector>

using namespace llvm;

static auto inlineCall(CallBase *callBase) -> bool
{
	llvm::InlineFunctionInfo ifi;
	return InlineFunction(*callBase, ifi).isSuccess();
}

/* Inlines F at its direct call sites (found through its uses, not by a
 * scan of the module). A call of F by itself stays */
static auto inlineFunction(Function &F) -> bool
{
	std::vector<CallBase *> calls;
	for (auto *user : F.users()) {
		auto *callBase = dyn_cast<CallBase>(user);
		if (callBase && callBase->getCalledFunction() == &F && callBase->getFunction() != &F)
			calls.push_back(callBase);
	}

	auto changed = false;
	for (auto *ci : calls) {
		changed |= inlineCall(ci);
	}
	return changed;
}

/*
 * Do not inline any functions that are (mutually) recursive.
 *
 * Example transforms that are permitted (below only f1 will be inlined):
 * f->f1->f2->f3->f4->f2->f3->f4... => f->f2->f3->f4->f2->f3->f4
 * f->f1->f2->f2->f2->... => main->f2->f2->f2...
 *
 * The functions are inlined callees first (the call graph's SCCs bottom-up):
 * a function's body is final by the time it is inlined, so its copies bring
 * no new call sites to inline, and each function is visited once. (Any order
 * gives the same result; this one does it in linear time.)
 */
auto FunctionInlinerPass::run(Module &M, ModuleAnalysisManager & /*AM*/) -> PreservedAnalyses
{
	CallGraph CG(M);
	auto changed = false;
	for (auto sccIt = scc_begin(&CG); !sccIt.isAtEnd(); ++sccIt) {
		/* A cyclic SCC is (mutually) recursive */
		if (sccIt.hasCycle())
			continue;
		for (auto *node : *sccIt) {
			auto *F = node->getFunction();
			/* Skip functions with empty bodies, external declarations
			 * and GenMC's own functions */
			if (F == nullptr || F->empty() || F->isDeclaration() ||
			    isInternalFunction(F->getName().str()))
				continue;
			changed |= inlineFunction(*F);
		}
	}
	return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
