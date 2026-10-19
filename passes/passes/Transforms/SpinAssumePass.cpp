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

#include "SpinAssumePass.hpp"
#include "genmc/ADT/VSet.hpp"
#include "genmc/Execution/EventAttr.hpp"
#include "genmc/Execution/LoadAnnotation.hpp"
#include "genmc/Support/Error.hpp"
#include "genmc/Support/SExpr.hpp"
#include "genmc/Support/SExprVisitor.hpp"
#include "genmc/Support/SVal.hpp"
#include "passes/InternalFunctions.hpp"
#include "passes/LLVMUtils.hpp"
#include "passes/Transforms/CallInfoCollectionPass.hpp"
#include "passes/Transforms/EscapeCheckerPass.hpp"
#include "passes/Transforms/InstAnnotator.hpp"

#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>
#include <llvm/Support/AtomicOrdering.h>
#include <llvm/Support/Casting.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/LoopUtils.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <ranges>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace llvm;

#define POSTDOM_PASS PostDominatorTreeWrapperPass
#define GET_POSTDOM_PASS() getAnalysis<POSTDOM_PASS>().getPostDomTree();

static void getLoopCASs(const Loop *l, SmallVector<const AtomicCmpXchgInst *, 4> &cass)
{
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	for (const auto *bb = l->block_begin(); bb != l->block_end(); ++bb) {
		for (auto i = (*bb)->begin(); i != (*bb)->end(); ++i) {
			if (auto *casi = dyn_cast<AtomicCmpXchgInst>(i))
				cass.push_back(casi);
		}
	}
}

static auto isBlockPHIClean(const BasicBlock *bb) -> bool { return !isa<PHINode>(&*bb->begin()); }

static auto accessSameVariable(const Value *ptr1, const Value *ptr2) -> bool
{
	/* Check if they are trivially the same */
	if (ptr1 == ptr2)
		return true;

	/* Check if they come from identical instructions */
	if (const auto *i1 = dyn_cast<Instruction>(ptr1))
		if (const auto *i2 = dyn_cast<Instruction>(ptr2))
			if (i1->isIdenticalTo(i2))
				return true;

	// FIXME: Is this really necessary?
	/* Special case: check if they are produced by bitcasts */
	if (const auto *bi1 = dyn_cast<BitCastInst>(ptr1)) {
		if (bi1->getOperand(0) == ptr2)
			return true;
		if (const auto *bi2 = dyn_cast<BitCastInst>(ptr2)) {
			if (ptr1 == bi2->getOperand(0))
				return true;
			if (bi1->getOperand(0) == bi2->getOperand(0))
				return true;
		}
	}
	return false;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static auto isPHIRelatedToCASCmp(const PHINode *curr,
				 const SmallVector<const AtomicCmpXchgInst *, 4> &cass,
				 SmallVector<const PHINode *, 4> &phiChain,
				 VSet<const PHINode *> &related) -> bool
{
	/* Check if we have already decided (or assumed) this phi is good */
	if (related.contains(curr) || std::ranges::find(phiChain, curr) != phiChain.end())
		return true;

	for (Value *val : curr->incoming_values()) {
		val = stripCasts(val);
		if (isa<UndefValue>(val)) {
			continue;
		}
		if (auto *li = dyn_cast<LoadInst>(val)) {
			if (std::ranges::all_of(cass, [&](const AtomicCmpXchgInst *casi) {
				    return !accessSameVariable(li->getPointerOperand(),
							       casi->getPointerOperand()) ||
					   !areSameLoadOrdering(li->getOrdering(),
								casi->getSuccessOrdering());
			    }))
				return false;
		} else if (auto *extract = dyn_cast<ExtractValueInst>(val)) {
			auto *ecasi = extractsFromCAS(extract);
			if (!ecasi || *extract->idx_begin() != 0)
				return false;
			if (std::ranges::all_of(cass, [&](const AtomicCmpXchgInst *casi) {
				    return !accessSameVariable(ecasi->getPointerOperand(),
							       casi->getPointerOperand()) ||
					   !areSameLoadOrdering(ecasi->getSuccessOrdering(),
								casi->getSuccessOrdering());
			    }))
				return false;
		} else {
			auto *phi = dyn_cast<PHINode>(val);
			if (!phi)
				return false;

			phiChain.push_back(curr);
			if (!isPHIRelatedToCASCmp(phi, cass, phiChain, related))
				return false;
			phiChain.pop_back();
		}
	}
	return true;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static auto isPHIRelatedToCASRes(const PHINode *curr,
				 const SmallVector<const AtomicCmpXchgInst *, 4> &cass,
				 SmallVector<const PHINode *, 4> &phiChain,
				 VSet<const PHINode *> &related) -> bool
{
	/* Check if we have already decided (or assumed) this phi is good */
	if (related.contains(curr) || std::ranges::find(phiChain, curr) != phiChain.end())
		return true;

	for (Value *val : curr->incoming_values()) {
		val = stripCasts(val);
		if (auto *constant = dyn_cast<Constant>(val)) {
			if (auto *ci = dyn_cast<ConstantInt>(constant)) {
				if (!ci->isZero())
					return false;
			} else if (!isa<UndefValue>(constant)) {
				return false;
			}
		} else if (auto *extract = dyn_cast<ExtractValueInst>(val)) {
			auto *ecasi = extractsFromCAS(extract);
			if (!ecasi || *extract->idx_begin() != 1)
				return false;
			if (std::ranges::all_of(cass, [&](const AtomicCmpXchgInst *casi) {
				    return !accessSameVariable(ecasi->getPointerOperand(),
							       casi->getPointerOperand()) ||
					   !areSameLoadOrdering(ecasi->getSuccessOrdering(),
								casi->getSuccessOrdering());
			    }))
				return false;
		} else {
			auto *phi = dyn_cast<PHINode>(val);
			if (!phi)
				return false;

			phiChain.push_back(curr);
			if (!isPHIRelatedToCASRes(phi, cass, phiChain, related))
				return false;
			phiChain.pop_back();
		}
	}

	/* If the current PHI does not depend on any other PHI being good, then we mark it as such
	 */
	if (phiChain.empty())
		related.insert(curr);
	return true;
}

static auto isPHIRelatedToCASCmp(const PHINode *phi,
				 const SmallVector<const AtomicCmpXchgInst *, 4> &cass) -> bool
{
	VSet<const PHINode *> related;
	SmallVector<const PHINode *, 4> phiChain;

	return isPHIRelatedToCASCmp(phi, cass, phiChain, related);
}

static auto isPHIRelatedToCASRes(const PHINode *phi,
				 const SmallVector<const AtomicCmpXchgInst *, 4> &cass) -> bool
{
	VSet<const PHINode *> related;
	SmallVector<const PHINode *, 4> phiChain;

	return isPHIRelatedToCASRes(phi, cass, phiChain, related);
}

static auto isPHIRelatedToLoad(const PHINode *curr, Value *&loadPtr,
			       std::optional<AtomicOrdering> &loadOrd,
			       SmallVector<const PHINode *, 4> &phiChain,
			       VSet<const PHINode *> &related) -> bool
{
	/* Check if we have already decided (or assumed) this phi is good */
	if (related.contains(curr) || std::ranges::find(phiChain, curr) != phiChain.end())
		return true;

	for (Value *val : curr->incoming_values()) {
		val = stripCasts(val);
		if (auto *li = dyn_cast_or_null<LoadInst>(val)) {
			if (loadPtr && !accessSameVariable(li->getPointerOperand(), loadPtr))
				return false;
			if (loadOrd.has_value() &&
			    !areSameLoadOrdering(li->getOrdering(), *loadOrd))
				return false;
			loadPtr = li->getPointerOperand();
			loadOrd = li->getOrdering();
		} else {
			auto *phi = dyn_cast<PHINode>(val);
			if (!phi)
				return false;

			phiChain.push_back(curr);
			if (!isPHIRelatedToLoad(phi, loadPtr, loadOrd, phiChain, related))
				return false;
			phiChain.pop_back();
		}
	}
	return true;
}

static auto isPHIRelatedToLoad(const PHINode *phi) -> bool
{
	Value *loadPtr = nullptr; // NOLINT(misc-const-correctness)
	std::optional<AtomicOrdering> loadOrd;
	VSet<const PHINode *> related;
	SmallVector<const PHINode *, 4> phiChain;

	return isPHIRelatedToLoad(phi, loadPtr, loadOrd, phiChain, related);
}

/*
 * This function checks whether a PHI node is tied to some load or CAS ('good' PHI).
 * A 'good' PHI node has incoming values that are either 1) PHI nodes that have been
 * deemed 'good', 2) constants and results of loads/CASes, or 3) loads at the same
 * location as some CAS and the compare operands of some CAS.
 * To avoid circles between PHIs, whenever we try to see whether a PHI is good,
 * we keep the current path in <phiChain>; if a node is deemed good and the chain
 * is empty (i.e., it does not depend on another node being deemed good), it is
 * moved to <related>, which stores PHIs that are related to some CAS operation.
 * Good PHIs may still change from one iteration to the next (e.g., a failed CAS
 * writes the value it read into its expected argument), so the spin-end only
 * blocks iterations that leave all header PHIs unchanged.
 */
static auto areBlockPHIsRelatedToLoopCASs(const BasicBlock *bb, Loop *l) -> bool
{
	SmallVector<const AtomicCmpXchgInst *, 4> cass;

	getLoopCASs(l, cass);
	if (cass.empty()) {
		return std::all_of(bb->phis().begin(), bb->phis().end(),
				   [&](auto &phi) { return isPHIRelatedToLoad(&phi); });
	}

	return std::all_of(bb->phis().begin(), bb->phis().end(), [&](auto &phi) {
		return isPHIRelatedToCASCmp(&phi, cass) || isPHIRelatedToCASRes(&phi, cass) ||
		       isPHIRelatedToLoad(&phi);
	});
}

/*
 * Returns whether INST extracts from a CAS. If a second argument is provided, it is ensured
 * that the extract extracts from this particular CAS.
 */
static auto isCASExtract(Instruction *inst, AtomicCmpXchgInst *cas = nullptr) -> bool
{
	auto *extract = llvm::dyn_cast_or_null<ExtractValueInst>(inst);
	if (!extract)
		return false;

	if (!extract->getType()->isIntegerTy() || extract->getNumIndices() > 1 ||
	    *extract->idx_begin() != 1)
		return false;

	auto *ecasi = dyn_cast<AtomicCmpXchgInst>(extract->getAggregateOperand());
	if (!ecasi)
		return false;
	return (cas) ? (cas == ecasi) : true;
}

/*
 * Tries to extract extractinsts corresponding to the results of a list of CASes.
 * Returns whether that was possible or not.
 */
static auto tryGetCASResultExtracts(const std::vector<AtomicCmpXchgInst *> &cass,
				    std::vector<ExtractValueInst *> &extracts) -> bool
{
	for (const auto &cas : cass) {
		/* Try both subsequent instructions as we might have
		 * eliminated one of them */
		if (isCASExtract(cas->getNextNode()->getNextNode(), cas)) {
			extracts.push_back(llvm::dyn_cast<ExtractValueInst>(
				cas->getNextNode()->getNextNode()));
		} else if (isCASExtract(cas->getNextNode(), cas)) {
			extracts.push_back(llvm::dyn_cast<ExtractValueInst>(cas->getNextNode()));
		} else {
			return false;
		}
	}
	return true;
}

/*
 * Returns whether the CASes of a backedge LATCH -> header(L) will lead to the loop header if
 * they fail. (If we exit the backedge paths with any of the CASes being successful, this function
 * will return false.)
 */
static auto failedCASesLeadToHeader(const std::vector<AtomicCmpXchgInst *> &cass, BasicBlock *latch,
				    Loop *l, const CallAnalysisResult::CallSet &cleanSet) -> bool
{
	if (cass.empty())
		return true;

	std::vector<ExtractValueInst *> extracts;

	if (!tryGetCASResultExtracts(cass, extracts))
		return false;

	std::vector<std::unique_ptr<SExpr<Value *>>> casConditions;
	casConditions.reserve(cass.size());
	for (auto *cas : cass)
		casConditions.push_back(
			InstAnnotator().annotateCASWithBackedgeCond(cas, latch, l, &cleanSet));

	auto backedgeCondition = ConjunctionExpr<Value *>::create(std::move(casConditions));
	for (auto i = 1U; std::cmp_less(i, (1 << extracts.size())); i++) {

		std::unordered_map<Value *, SVal> valueMap;
		for (auto j = 0U; j < extracts.size(); j++)
			valueMap[extracts[j]] = (i & (1 << j)) ? SVal(1) : SVal(0);

		size_t unknowns = 0;
		auto res = SExprEvaluator<Value *>().evaluate(backedgeCondition.get(), valueMap,
							      &unknowns);
		if (unknowns > 0 || res.getBool())
			return false;
	}
	return true;
}

static auto isStoreLocal(StoreInst *si, EscapeAnalysisResult &EAR, DominatorTree &DT) -> bool
{
	/* A store is local if it is either marked or writes to dynamic memory */
	auto attr = getWriteAttr(*si);
	auto *alloc = EAR.writesDynamicMemory(si->getPointerOperand());
	return (alloc && EAR.escapesAfter(alloc, si, DT)) || !!(attr & WriteAttr::Local);
}

static auto isPathToHeaderEffectFree(BasicBlock *latch, Loop *l, ModuleAnalysisManager &MAM,
				     bool &checkDynamically) -> bool
{
	auto &FAM =
		MAM.getResult<FunctionAnalysisManagerModuleProxy>(*latch->getParent()->getParent())
			.getManager();
	auto &EAR = MAM.getResult<EscapeAnalysis>(*latch->getParent()->getParent());
	auto &CAR = MAM.getResult<CallAnalysis>(*latch->getParent()->getParent());
	auto &DT = FAM.getResult<DominatorTreeAnalysis>(*latch->getParent());
	auto &cleanSet = CAR.clean;

	auto effects = false;
	std::vector<AtomicCmpXchgInst *> cass;

	foreachInBackPathTo(latch, l->getHeader(), [&](Instruction &i) {
		/* Try to prove that failed CASes imply another iteration */
		if (auto *casi = dyn_cast<AtomicCmpXchgInst>(&i)) {
			cass.push_back(casi);
			return;
		}
		/* Local stores are allowed */
		if (auto *si = dyn_cast<StoreInst>(&i)) {
			if (isStoreLocal(si, EAR[latch->getParent()], DT))
				return;
		}
		effects |= hasSideEffects(&i, &cleanSet);
	});
	if (effects) {
		checkDynamically = true;
		return false;
	}

	std::ranges::sort(cass);
	cass.erase(std::ranges::unique(cass).begin(), cass.end());

	auto casOK = true;
	if (!cass.empty()) {
		casOK = failedCASesLeadToHeader(cass, latch, l, cleanSet);
		checkDynamically |= !casOK;
	}
	return casOK;
}

template <typename F>
static auto checkConstantsCondition(const Value *v1, const Value *v2, F &&cond) -> bool
{
	const auto *ci1 = dyn_cast<ConstantInt>(v1);
	const auto *ci2 = dyn_cast<ConstantInt>(v2);

	if (!ci1 || !ci2)
		return false;

	return std::forward<F>(cond)(ci1->getValue(), ci2->getValue());
}

static auto areCancelingBinops(const AtomicRMWInst *rmw1, const AtomicRMWInst *rmw2) -> bool
{
	using namespace llvm;
	using BinOp = AtomicRMWInst::BinOp;

	auto op1 = rmw1->getOperation();
	auto op2 = rmw2->getOperation();
	const auto *v1 = rmw1->getValOperand();
	const auto *v2 = rmw2->getValOperand();

	/* The two operations need to manipulate the same memory location */
	if (!accessSameVariable(rmw1->getPointerOperand(), rmw2->getPointerOperand()))
		return false;

	/* The operators need to be opposite and the values the same, or vice versa */
	if (((op1 == BinOp::Add && op2 == BinOp::Sub) ||
	     (op1 == BinOp::Sub && op2 == BinOp::Add)) &&
	    checkConstantsCondition(v1, v2,
				    [&](const APInt &i1, const APInt &i2) { return i1 == i2; }))
		return true;

	bool overflow = false;
	return op1 == op2 && (op1 == BinOp::Add || op1 == BinOp::Sub) &&
	       checkConstantsCondition(v1, v2, [&](const APInt &i1, const APInt &i2) {
		       return i1.sadd_ov(i2, overflow) == 0;
	       });
}

static auto dominatesAndPostdominates(Instruction *instA, Instruction *instB, DominatorTree &DT,
				      PostDominatorTree &PDT) -> bool
{
	return DT.dominates(instA, instB) && PDT.dominates(instA->getParent(), instB->getParent());
}

static auto isPathToHeaderFAIZNE(BasicBlock *latch, Loop *l, ModuleAnalysisManager &MAM,
				 Instruction *&lastEffect) -> bool
{
	auto &FAM =
		MAM.getResult<FunctionAnalysisManagerModuleProxy>(*latch->getParent()->getParent())
			.getManager();
	auto &CAR = MAM.getResult<CallAnalysis>(*latch->getParent()->getParent());
	auto &DT = FAM.getResult<DominatorTreeAnalysis>(*latch->getParent());
	auto &PDT = FAM.getResult<PostDominatorTreeAnalysis>(*latch->getParent());
	auto &cleanSet = CAR.clean;

	auto effects = false;
	VSet<AtomicCmpXchgInst *> cass;
	VSet<AtomicRMWInst *> fais;

	foreachInBackPathTo(latch, l->getHeader(), [&](Instruction &i) {
		if (auto *faii = dyn_cast<AtomicRMWInst>(&i)) {
			fais.insert(faii);
			return;
		}
		/* CASes are OK as long as they are before the decrement */
		if (auto *casi = dyn_cast<AtomicCmpXchgInst>(&i)) {
			cass.insert(casi);
			return;
		}
		effects |= hasSideEffects(&i, &cleanSet);
	});

	if (effects || fais.size() != 2)
		return false;

	/* Check domination conditions; we need more checks here due to the (unordered) VSet */

	const AtomicRMWInst *inci = nullptr;
	AtomicRMWInst *deci = nullptr;
	if (DT.dominates(fais[0], fais[1]) && PDT.dominates(l->getHeader(), fais[1]->getParent())) {
		inci = fais[0];
		deci = fais[1];
	} else if (DT.dominates(fais[1], fais[0]) &&
		   PDT.dominates(l->getHeader(), fais[0]->getParent())) {
		inci = fais[1];
		deci = fais[0];
	}
	if (!inci || !deci)
		return false;
	if (std::ranges::any_of(cass,
				[&](AtomicCmpXchgInst *casi) { return !DT.dominates(casi, deci); }))
		return false;

	/* Check cancelation */
	if (!areCancelingBinops(inci, deci))
		return false;

	lastEffect = deci;
	return true;
}

static auto isPathToHeaderLockZNE(BasicBlock *latch, Loop *l, ModuleAnalysisManager &MAM,
				  Instruction *&lastEffect) -> bool
{
	auto &FAM =
		MAM.getResult<FunctionAnalysisManagerModuleProxy>(*latch->getParent()->getParent())
			.getManager();
	auto &CAR = MAM.getResult<CallAnalysis>(*latch->getParent()->getParent());
	auto &DT = FAM.getResult<DominatorTreeAnalysis>(*latch->getParent());
	auto &PDT = FAM.getResult<PostDominatorTreeAnalysis>(*latch->getParent());
	auto &cleanSet = CAR.clean;

	auto effects = false;
	VSet<CallInst *> locks;
	VSet<CallInst *> unlocks;
	VSet<PHINode *> phis;

	foreachInBackPathTo(latch, l->getHeader(), [&](Instruction &i) {
		if (auto *ci = dyn_cast<CallInst>(&i)) {
			auto name = getCalledFunOrStripValName(*ci);
			if (isInternalFunction(name)) {
				auto icode = internalFunNames.at(name);
				if (icode == InternalFunctions::MutexLock) {
					locks.insert(ci);
					return;
				}
				if (icode == InternalFunctions::MutexUnlock) {
					unlocks.insert(ci);
					return;
				}
			}
		}
		if (auto *phi = dyn_cast<PHINode>(&i)) {
			if (phi->getParent() == l->getHeader()) /* PHIs in the body are OK */
				phis.insert(phi);
			return;
		}
		effects |= hasSideEffects(&i, &cleanSet);
	});

	if (effects || locks.size() != 1 || unlocks.size() != 1 || !phis.empty())
		return false;

	auto lDomU = dominatesAndPostdominates(locks[0], unlocks[0], DT, PDT);
	if (!lDomU || !accessSameVariable(*locks[0]->arg_begin(), *unlocks[0]->arg_begin()))
		return false;

	lastEffect = unlocks[0];
	return true;
}

static auto getOrCreateExitingCondition(BasicBlock *header, Instruction *term) -> Value *
{
	if (isa<IndirectBrInst>(term))
		return ConstantInt::getFalse(term->getContext());

	auto *bi = dyn_cast<BranchInst>(term);
	VERIFY(bi);

	if (bi->isUnconditional())
		return ConstantInt::getFalse(term->getContext());
	if (bi->getSuccessor(0) != header)
		return bi->getCondition();
	return BinaryOperator::CreateNot(bi->getCondition(), "", term->getIterator());
}

static void addLoopBeginCallBeforeTerm(BasicBlock *preheader)
{
	auto *term = preheader->getTerminator();
	auto *beginFun = preheader->getParent()->getParent()->getFunction("__VERIFIER_loop_begin");
	VERIFY(beginFun);

	auto *ci = CallInst::Create(beginFun, {}, "", term->getIterator());
	ci->setMetadata("dbg", term->getMetadata("dbg"));
}

/* Whether the header PHIs can be compared for createHeaderChangeCondition() */
static auto areHeaderPHIsComparable(BasicBlock *header) -> bool
{
	return std::ranges::all_of(header->phis(), [](PHINode &phi) {
		return phi.getType()->isIntegerTy() || phi.getType()->isPointerTy() ||
		       phi.getType()->isFloatingPointTy();
	});
}

static auto canBackedgeChangeHeader(BasicBlock *header, BasicBlock *pred) -> bool
{
	return std::ranges::any_of(header->phis(), [&](PHINode &phi) {
		return phi.getIncomingValueForBlock(pred) != &phi;
	});
}

/*
 * Returns a condition that holds if the edge PRED -> HEADER changes the value of a PHI
 * of HEADER, i.e., if the next iteration does not repeat the current one. Returns null
 * if no PHI can change. Instructions are inserted before POS.
 */
static auto createHeaderChangeCondition(BasicBlock *header, BasicBlock *pred,
					BasicBlock::iterator pos) -> Value *
{
	Value *changed = nullptr;
	for (auto &phi : header->phis()) {
		Value *curr = &phi;
		Value *next = phi.getIncomingValueForBlock(pred);
		if (next == curr)
			continue;
		if (curr->getType()->isFloatingPointTy()) {
			/* Compare the bits: the PHI changes, or it does not */
			auto *intTy = IntegerType::get(header->getContext(),
						       curr->getType()->getPrimitiveSizeInBits());
			curr = CastInst::CreateBitOrPointerCast(curr, intTy, "", pos);
			next = CastInst::CreateBitOrPointerCast(next, intTy, "", pos);
		}
		auto *ne =
			CmpInst::Create(Instruction::ICmp, CmpInst::ICMP_NE, next, curr, "", pos);
		changed = changed ? BinaryOperator::CreateOr(changed, ne, "", pos)
				  : static_cast<Value *>(ne);
	}
	return changed;
}

/*
 * Makes the backedge LATCH -> header of L call __VERIFIER_loop_begin() when it changes a
 * header PHI. A spin-start blocks the thread if the previous iteration had no side
 * effects; an iteration that changed a header PHI is not repeated by the next one,
 * which therefore starts the loop anew. (This also restarts the tracking of the
 * loop's reads, which decides whether a store to thread-local memory counts as a
 * side effect; a blocked iteration repeats the previous one, stores included.)
 */
static void addLoopBeginOnHeaderChange(BasicBlock *latch, Loop *l, DominatorTree &DT, LoopInfo &LI)
{
	auto *header = l->getHeader();
	if (!canBackedgeChangeHeader(header, latch))
		return;

	auto *dbg = latch->getTerminator()->getMetadata("dbg");
	auto *edge = SplitEdge(latch, header, &DT, &LI);
	auto *changed =
		createHeaderChangeCondition(header, edge, edge->getTerminator()->getIterator());
	auto *begin = BasicBlock::Create(header->getContext(), "spin.changed", header->getParent(),
					 header);
	BranchInst::Create(header, begin)->setMetadata("dbg", dbg);
	addLoopBeginCallBeforeTerm(begin);
	edge->getTerminator()->eraseFromParent();
	BranchInst::Create(begin, header, changed, edge)->setMetadata("dbg", dbg);
	for (auto &phi : header->phis())
		phi.addIncoming(phi.getIncomingValueForBlock(edge), begin);
	DT.addNewBlock(begin, edge);
	l->addBasicBlockToLoop(begin, LI);
}

/*
 * Blocks the thread at the end of LATCH if the loop does not exit and the next iteration
 * would repeat the current one (no header PHI changes).
 */
static void addSpinEndCallBeforeTerm(BasicBlock *latch, BasicBlock *header)
{
	auto *term = latch->getTerminator();
	auto *endFun = latch->getParent()->getParent()->getFunction("__VERIFIER_assume_internal");
	VERIFY(endFun);

	auto *cond = getOrCreateExitingCondition(header, term);
	if (auto *changed = createHeaderChangeCondition(header, latch, term->getIterator())) {
		auto *ci = dyn_cast<ConstantInt>(cond);
		cond = (ci && ci->isZero())
			       ? changed
			       : BinaryOperator::CreateOr(cond, changed, "", term->getIterator());
	}
	auto *assumeType = ConstantInt::get(
		IntegerType::get(term->getContext(), 8),
		APInt(8, static_cast<std::underlying_type_t<AssumeType>>(AssumeType::Spinloop)));
	auto *ci = CallInst::Create(endFun, {cond, assumeType}, "", term->getIterator());
	ci->setMetadata("dbg", term->getMetadata("dbg"));
}

/*
 * Makes V available at POS: V is already, or is a pure computation after POS
 * in POS's block (e.g., the comparison of a loop's exit branch) whose
 * operands are, and is moved before POS. Returns whether V is available.
 */
static auto hoistAbove(Value *v, Instruction *pos, DominatorTree &DT, unsigned depth = 0) -> bool
{
	auto *i = dyn_cast<Instruction>(v);
	if (!i || DT.dominates(i, pos))
		return true;
	if (depth > 8 || i->getParent() != pos->getParent() || isa<PHINode>(i) ||
	    i->mayReadOrWriteMemory() || i->mayHaveSideEffects() || i->isTerminator())
		return false;
	if (!std::ranges::all_of(i->operands(),
				 [&](Use &op) { return hoistAbove(op.get(), pos, DT, depth + 1); }))
		return false;
	i->moveBefore(pos->getIterator());
	return true;
}

/*
 * A ZNE spin end blocks the thread before LASTEFFECT (the decrementing FAI, or
 * the unlock), and must do so only if the iteration goes back to the header:
 * returns the condition that it does, valid at LASTEFFECT, or nullptr if it
 * cannot be computed there. The loop may still exit after LASTEFFECT through
 * one exiting block that post-dominates LASTEFFECT and whose condition is
 * computed before LASTEFFECT. (A path from LASTEFFECT back to the header
 * that avoids that block cannot exit; the condition may then say "exits"
 * and leave the iteration unblocked, which costs exploration, not results.)
 */
static auto getZNEContinueCondition(Loop *l, Instruction *lastEffect, ModuleAnalysisManager &MAM)
	-> Value *
{
	auto *header = l->getHeader();
	auto *start = lastEffect->getParent();
	auto &FAM = MAM.getResult<FunctionAnalysisManagerModuleProxy>(*start->getParent()->getParent())
			    .getManager();
	auto &DT = FAM.getResult<DominatorTreeAnalysis>(*start->getParent());
	auto &PDT = FAM.getResult<PostDominatorTreeAnalysis>(*start->getParent());

	/* The exiting blocks reachable from LASTEFFECT within the iteration */
	SmallVector<BasicBlock *, 4> exiting;
	SmallVector<BasicBlock *, 8> worklist = {start};
	VSet<BasicBlock *> seen;
	seen.insert(start);
	while (!worklist.empty()) {
		auto *bb = worklist.pop_back_val();
		for (auto *succ : successors(bb)) {
			if (!l->contains(succ)) {
				if (exiting.empty() || exiting.back() != bb)
					exiting.push_back(bb);
				continue;
			}
			if (succ == header || seen.count(succ))
				continue;
			seen.insert(succ);
			worklist.push_back(succ);
		}
	}
	if (exiting.empty())
		return ConstantInt::getTrue(lastEffect->getContext());
	if (exiting.size() != 1)
		return nullptr;

	auto *bb = exiting[0];
	auto *bi = dyn_cast<BranchInst>(bb->getTerminator());
	if (!bi || bi->isUnconditional() || (bb != start && !PDT.dominates(bb, start)))
		return nullptr;
	auto *cond = bi->getCondition();
	if (!hoistAbove(cond, lastEffect, DT))
		return nullptr;
	auto staysOn0 = l->contains(bi->getSuccessor(0));
	if (staysOn0 == l->contains(bi->getSuccessor(1)))
		return nullptr;
	return staysOn0 ? cond : BinaryOperator::CreateNot(cond, "", lastEffect->getIterator());
}

static void addPotentialSpinEndCall(StringRef name, Instruction *lastEffect, Value *cont)
{
	auto *endFun = lastEffect->getParent()->getParent()->getParent()->getFunction(name);
	VERIFY(endFun);

	auto *ci = CallInst::Create(endFun, {cont}, "", lastEffect->getIterator());
	ci->setMetadata("dbg", lastEffect->getMetadata("dbg"));
}

static void addPotentialSpinEndCallBeforeLastFai(Instruction *lastEffect, Value *cont)
{
	addPotentialSpinEndCall("__VERIFIER_faiZNE_spin_end", lastEffect, cont);
}

static void addPotentialSpinEndCallBeforeUnlock(Instruction *lastEffect, Value *cont)
{
	addPotentialSpinEndCall("__VERIFIER_lockZNE_spin_end", lastEffect, cont);
}

static void addSpinStartCall(Loop *l)
{
	auto *startFun =
		l->getHeader()->getParent()->getParent()->getFunction("__VERIFIER_spin_start");

	auto i = l->getHeader()->getFirstNonPHIIt();
	CallInst::Create(startFun, {}, "", i);
}

/*
 * Returns whether VAL, which flows into the header PHI PHI of L along a backedge, is
 * either PHI itself or a value read inside L from the location of INIT with INIT's
 * ordering: a load, or the old value of a CAS, which a failed CAS reads with its
 * failure ordering.
 */
static auto isLoopReadLike(Value *val, const PHINode *phi, const Loop *l, const LoadInst *init,
			   VSet<const PHINode *> &visited) -> bool
{
	if (val == phi)
		return true;

	auto *inst = dyn_cast<Instruction>(val);
	if (!inst || !l->contains(inst))
		return false;

	const auto *ptr = init->getPointerOperand();
	if (auto *li = dyn_cast<LoadInst>(inst))
		return !li->isVolatile() && accessSameVariable(li->getPointerOperand(), ptr) &&
		       areSameLoadOrdering(li->getOrdering(), init->getOrdering());
	if (auto *extract = dyn_cast<ExtractValueInst>(inst)) {
		auto *casi = dyn_cast<AtomicCmpXchgInst>(extract->getAggregateOperand());
		return casi && l->contains(casi) && !casi->isVolatile() &&
		       extract->getNumIndices() == 1 && *extract->idx_begin() == 0 &&
		       accessSameVariable(casi->getPointerOperand(), ptr) &&
		       areSameLoadOrdering(casi->getFailureOrdering(), init->getOrdering());
	}
	if (auto *p = dyn_cast<PHINode>(inst)) {
		if (visited.contains(p))
			return true;
		visited.insert(p);
		return std::ranges::all_of(p->incoming_values(), [&](Value *v) {
			return isLoopReadLike(v, phi, l, init, visited);
		});
	}
	return false;
}

/*
 * A CAS retry loop that refreshes its expected value with the value it read,
 *     old = load(x); while (!cas(&x, &old, f(old)));
 * carries that value in a header PHI, so a failed iteration is not repeated by the
 * next one: the retry compares against what the failed CAS read. If nothing can tell
 * the initial load apart from a load at the top of the loop, this moves the load
 * there, so that each iteration reads x afresh:
 *     do { old = load(x); } while (!cas(&x, &old, f(old)));
 * Then a failed iteration is pruned like in any other spinloop. This is sound: an
 * execution where the loop fails k times and then exits corresponds to one where the
 * load reads the value that the last failed CAS (or a reload) read, followed by the
 * exiting iteration. The conditions make that execution a valid one with the same
 * outcome:
 *   - the PHI is the only one in the header, and every backedge carries a spin-end
 *     (so that all failed iterations are pruned);
 *   - its value from the preheader is an atomic load of x whose only user is the PHI,
 *     and nothing after the load in the preheader accesses memory;
 *   - every value it gets on a backedge is read from x inside the loop, by a load or
 *     a CAS, with the ordering of the initial load.
 * (The same holds for a loop that refreshes the value with a load and has no CAS.)
 * Returns whether the loop was rewritten.
 */
static auto tryMoveInitialLoadToHeader(Loop *l, ModuleAnalysisManager &MAM) -> bool
{
	auto *header = l->getHeader();
	auto *preheader = l->getLoopPreheader();
	if (!preheader || isBlockPHIClean(header) ||
	    std::next(header->phis().begin()) != header->phis().end())
		return false;

	auto *phi = &*header->phis().begin();
	auto *init = dyn_cast<LoadInst>(phi->getIncomingValueForBlock(preheader));
	if (!init || init->getParent() != preheader || !init->isAtomic() || init->isVolatile() ||
	    !init->hasOneUse())
		return false;
	if (std::any_of(std::next(init->getIterator()), preheader->end(), [](Instruction &i) {
		    return i.mayReadOrWriteMemory() || i.mayHaveSideEffects();
	    }))
		return false;

	SmallVector<BasicBlock *, 4> latches;
	l->getLoopLatches(latches);
	VSet<const PHINode *> visited;
	if (!std::ranges::all_of(latches, [&](BasicBlock *latch) {
		    Instruction *lastEffect = nullptr;
		    auto checkDynamically = false;
		    return isLoopReadLike(phi->getIncomingValueForBlock(latch), phi, l, init,
					  visited) &&
			   !isPathToHeaderFAIZNE(latch, l, MAM, lastEffect) &&
			   !isPathToHeaderLockZNE(latch, l, MAM, lastEffect) &&
			   isPathToHeaderEffectFree(latch, l, MAM, checkDynamically);
	    }))
		return false;

	/* Move (rather than recreate) the load: analyses may refer to it */
	init->moveBefore(*header, header->getFirstNonPHIIt());
	phi->replaceAllUsesWith(init);
	phi->eraseFromParent();
	return true;
}

static auto checkLoop(Loop *l, ModuleAnalysisManager &MAM, bool markStarts) -> bool
{
	auto *header = l->getHeader();

	auto moved = tryMoveInitialLoadToHeader(l, MAM);
	auto hasPHIs = !isBlockPHIClean(header);
	if (hasPHIs &&
	    (!areBlockPHIsRelatedToLoopCASs(header, l) || !areHeaderPHIsComparable(header)))
		return false;

	SmallVector<BasicBlock *, 4> latches;
	l->getLoopLatches(latches);

	auto modified = false;
	auto checkDynamically = false;
	llvm::Instruction *lastZNEEffect = nullptr;
	llvm::Value *cont = nullptr;
	for (auto &latch : latches) {
		/* FAI-ZNE blocks ignore header PHIs (lock-ZNE requires none);
		 * with PHIs such paths are checked dynamically. A ZNE spin end
		 * blocks only if the iteration does not exit after it */
		if (!hasPHIs && isPathToHeaderFAIZNE(latch, l, MAM, lastZNEEffect) &&
		    (cont = getZNEContinueCondition(l, lastZNEEffect, MAM))) {
			checkDynamically = true;
			modified = true;
			addPotentialSpinEndCallBeforeLastFai(lastZNEEffect, cont);
		} else if (isPathToHeaderLockZNE(latch, l, MAM, lastZNEEffect) &&
			   (cont = getZNEContinueCondition(l, lastZNEEffect, MAM))) {
			/* Check for lockZNE before effect-free paths,
			 * as locks are function calls too...  */
			checkDynamically = true;
			modified = true;
			addPotentialSpinEndCallBeforeUnlock(lastZNEEffect, cont);
		} else if (isPathToHeaderEffectFree(latch, l, MAM, checkDynamically)) {
			/* If we have to dynamically validate the loop,
			 * the above check will return false, but the path
			 * may be checked dynamically */
			addSpinEndCallBeforeTerm(latch, header);
			modified = true; /* At the very least, a spin_start is inserted */
		}
	}

	/* Mark spinloop start if we have to */
	if (checkDynamically || (modified && markStarts)) {
		addSpinStartCall(l);
		/* DSA also requires us to know when we actually enter the loop;
		 * mark the beginning anyway to compose with liveness checks and nested loops */
		auto &FAM = MAM.getResult<FunctionAnalysisManagerModuleProxy>(
				       *header->getParent()->getParent())
				    .getManager();
		auto &DT = FAM.getResult<DominatorTreeAnalysis>(*header->getParent());
		auto &LI = FAM.getResult<LoopAnalysis>(*header->getParent());
		auto *preheader = llvm::InsertPreheaderForLoop(l, &DT, &LI, nullptr, false);
		addLoopBeginCallBeforeTerm(preheader);
		if (hasPHIs) {
			for (auto *latch : latches)
				addLoopBeginOnHeaderChange(latch, l, DT, LI);
		}
	}

	/* Two cases:
	 * 1) If the transformation applied did not apply in all backedges, this is indeed a loop
	 * 2) An actual spinloop: we used to remove it from the loop list but let's keep it */
	return modified || moved;
}

auto SpinAssumePass::run(Module &M, ModuleAnalysisManager &MAM) const -> PreservedAnalyses
{
	auto &FAM = MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();
	auto modified = false;
	for (auto &F : M | std::views::filter([&](auto &F) { return !F.isDeclaration(); })) {
		auto &LI = FAM.getResult<LoopAnalysis>(F);
		for (auto &L : std::views::reverse(LI.getLoopsInPreorder())) {
			modified |= checkLoop(L, MAM, markStarts_);
		}
	}
	return modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
