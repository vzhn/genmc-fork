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

#include "PassChecker.hpp"
#include "genmc/Execution/LoadAnnotation.hpp"
#include "genmc/Support/Error.hpp"
#include "genmc/Support/SExpr.hpp"
#include "passes/InternalFunctions.hpp"
#include "passes/LLVMUtils.hpp"

#include <llvm/Analysis/CaptureTracking.h>
#include <llvm/ADT/MapVector.h>
#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/IR/CFG.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/AsmParser/Parser.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FormatAdapters.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/PromoteMemToReg.h>

#include <cctype>
#include <cstdint>
#include <memory>
#include <optional>
#include <set>

/* Which annotations EliminateAnnotationsPass turns into attributes */
struct PassChecker::AnnotationKinds {
	bool helper;
	bool confirmation;
	bool finalWrite;

	[[nodiscard]] auto kept(uint64_t v) const -> bool
	{
		if (v == GENMC_KIND_HELPED || v == GENMC_KIND_HELPING)
			return helper;
		if (v == GENMC_KIND_CONFIRM || v == GENMC_KIND_SPECUL)
			return confirmation;
		if (v == GENMC_ATTR_FINAL)
			return finalWrite;
		return true;
	}
};

namespace {
using AnnotationKinds = PassChecker::AnnotationKinds;

/* The function a pass ran on, if it ran on a function or a loop */
auto getPassFunction(llvm::Any IR) -> llvm::Function *
{
	if (const auto *F = llvm::any_cast<const llvm::Function *>(&IR))
		return const_cast<llvm::Function *>(*F);
	if (const auto *L = llvm::any_cast<const llvm::Loop *>(&IR))
		return (*L)->getHeader()->getParent();
	return nullptr;
}

/* The module a pass ran on */
auto getPassModule(llvm::Any IR) -> const llvm::Module *
{
	if (const auto *M = llvm::any_cast<const llvm::Module *>(&IR))
		return *M;
	if (auto *F = getPassFunction(IR))
		return F->getParent();
	return nullptr;
}

/* Pass managers and adaptors: they only run other passes */
auto isPassContainer(llvm::StringRef passID) -> bool
{
	return passID.contains("PassManager") || passID.contains("PassAdaptor");
}

auto printModule(const llvm::Module &mod) -> std::string
{
	std::string s;
	llvm::raw_string_ostream os(s);
	mod.print(os, nullptr);
	return s;
}

/* A name for TYPE in a function name */
auto typeTag(llvm::Type *type) -> std::string
{
	std::string s;
	llvm::raw_string_ostream os(s);
	type->print(os);
	for (auto &c : s)
		if (!std::isalnum(static_cast<unsigned char>(c)))
			c = '_';
	return s;
}

auto isInternalCall(const llvm::Instruction &I, InternalFunctions code) -> bool
{
	auto *ci = llvm::dyn_cast<llvm::CallInst>(&I);
	if (!ci)
		return false;
	auto name = getCalledFunOrStripValName(*ci);
	return isInternalFunction(name) && internalFunNames.at(name) == code;
}

auto isMutexCall(const llvm::Instruction &I) -> bool
{
	auto *ci = llvm::dyn_cast<llvm::CallInst>(&I);
	if (!ci)
		return false;
	auto name = getCalledFunOrStripValName(*ci);
	return isInternalFunction(name) && isMutexCode(internalFunNames.at(name));
}

/* An atomic access to a local whose address does not escape: no other
 * thread sees it, and a pass may promote it like a plain one */
auto isPrivateAccess(const llvm::Instruction &I) -> bool
{
	const llvm::Value *ptr = nullptr;
	if (const auto *L = llvm::dyn_cast<llvm::LoadInst>(&I))
		ptr = L->getPointerOperand();
	else if (const auto *S = llvm::dyn_cast<llvm::StoreInst>(&I))
		ptr = S->getPointerOperand();
	if (!ptr)
		return false;
	const auto *obj = llvm::dyn_cast<llvm::AllocaInst>(llvm::getUnderlyingObject(ptr));
	return obj && !llvm::PointerMayBeCaptured(obj, /*ReturnCaptures=*/true);
}

/*
 * The annotations of accesses as observable calls. EliminateAnnotationsPass
 * turns __VERIFIER_annotate_begin(v) ... __VERIFIER_annotate_end(v) into a
 * genmc.attr of v on the atomic accesses and mutex calls in between. Here a
 * variable holds the annotations open at each point (begin adds v, end
 * removes it), and each such instruction is preceded by a call
 * __alive_attr(open | its genmc.attr): the module before the pass and the
 * one after agree on these calls iff each instruction got the annotations
 * open where it runs (and a later pass keeps them).
 */
void exposeAnnotations(llvm::Module &mod, const AnnotationKinds &kinds)
{
	auto &ctx = mod.getContext();
	auto *i64 = llvm::Type::getInt64Ty(ctx);
	auto attrFun = mod.getOrInsertFunction(
		"__alive_attr", llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), {i64}, false));
	auto value = [](llvm::CallInst *ci) {
		return llvm::cast<llvm::ConstantInt>(ci->getArgOperand(0))->getZExtValue();
	};

	for (auto &F : mod) {
		if (F.isDeclaration())
			continue;
		auto *open = new llvm::AllocaInst(i64, 0, "annot.open",
						  F.getEntryBlock().getFirstInsertionPt());
		new llvm::StoreInst(llvm::ConstantInt::get(i64, 0), open,
				    std::next(open->getIterator()));
		llvm::SmallVector<llvm::Instruction *, 16> points;
		for (auto &I : llvm::instructions(F))
			if (isInternalCall(I, InternalFunctions::AnnotateBegin) ||
			    isInternalCall(I, InternalFunctions::AnnotateEnd) ||
			    (I.isAtomic() && !llvm::isa<llvm::FenceInst>(I) &&
			     !isPrivateAccess(I)) ||
			    isMutexCall(I))
				points.push_back(&I);
		for (auto *I : points) {
			auto it = I->getIterator();
			auto *cur = new llvm::LoadInst(i64, open, "", it);
			if (isInternalCall(*I, InternalFunctions::AnnotateBegin) ||
			    isInternalCall(*I, InternalFunctions::AnnotateEnd)) {
				auto *ci = llvm::cast<llvm::CallInst>(I);
				auto v = value(ci);
				if (kinds.kept(v)) {
					auto *upd =
						isInternalCall(*I, InternalFunctions::AnnotateBegin)
							? llvm::BinaryOperator::CreateOr(
								  cur,
								  llvm::ConstantInt::get(i64, v),
								  "", it)
							: llvm::BinaryOperator::CreateAnd(
								  cur,
								  llvm::ConstantInt::get(i64, ~v),
								  "", it);
					new llvm::StoreInst(upd, open, it);
				}
				I->eraseFromParent();
				continue;
			}
			llvm::Value *attr = cur;
			if (auto *md = I->getMetadata("genmc.attr")) {
				auto v = llvm::cast<llvm::ConstantInt>(
						 llvm::cast<llvm::ConstantAsMetadata>(
							 md->getOperand(0))
							 ->getValue())
						 ->getZExtValue();
				attr = llvm::BinaryOperator::CreateOr(
					cur, llvm::ConstantInt::get(i64, v), "", it);
			}
			llvm::CallInst::Create(attrFun, {attr}, "", it);
		}
	}
}

/* Inline the calls to internal functions defined in the module (Alive2
 * checks one function at a time, so an inlined call is otherwise a call to
 * an unknown function on one side only) */
void inlineInternalCalls(llvm::Module &mod)
{
	for (auto round = 0; round < 8; round++) {
		llvm::SmallVector<llvm::CallBase *, 16> calls;
		for (auto &F : mod)
			for (auto &I : llvm::instructions(F))
				if (auto *cb = llvm::dyn_cast<llvm::CallBase>(&I);
				    cb && cb->getCalledFunction() &&
				    !cb->getCalledFunction()->isDeclaration() &&
				    cb->getCalledFunction() != &F)
					calls.push_back(cb);
		if (calls.empty())
			return;
		for (auto *cb : calls) {
			llvm::InlineFunctionInfo IFI;
			llvm::InlineFunction(*cb, IFI);
		}
	}
}

/* The integer type an access of TYPE goes through: pointers become integers of
 * their size (GenMC accesses both the same way) */
auto accessType(llvm::Type *type, const llvm::DataLayout &DL) -> llvm::Type *
{
	if (type->isPointerTy())
		return llvm::IntegerType::get(type->getContext(), DL.getPointerSizeInBits());
	return type;
}

/* Call to the external function NAME with ARGS before POS */
auto createExternalCall(llvm::Instruction *pos, const std::string &name, llvm::Type *retTy,
			llvm::ArrayRef<llvm::Value *> args) -> llvm::CallInst *
{
	llvm::SmallVector<llvm::Type *, 4> argTys;
	for (auto *arg : args)
		argTys.push_back(arg->getType());
	auto *mod = pos->getModule();
	auto callee = mod->getOrInsertFunction(name, llvm::FunctionType::get(retTy, argTys, false));
	/* An access returns a value, never poison */
	if (auto *fun = llvm::dyn_cast<llvm::Function>(callee.getCallee());
	    fun && !retTy->isVoidTy())
		fun->addRetAttr(llvm::Attribute::NoUndef);
	return llvm::CallInst::Create(callee, args, "", pos->getIterator());
}

/* V as a value of TO (pointer <-> integer of the same size), before POS */
auto convert(llvm::Value *v, llvm::Type *to, llvm::Instruction *pos) -> llvm::Value *
{
	if (v->getType() == to)
		return v;
	if (to->isPointerTy())
		return new llvm::IntToPtrInst(v, to, "", pos->getIterator());
	return new llvm::PtrToIntInst(v, to, "", pos->getIterator());
}

/* Whether I is a volatile plain access (Alive2 has no volatile either) */
auto isVolatileAccess(const llvm::Instruction &I) -> bool
{
	if (const auto *L = llvm::dyn_cast<llvm::LoadInst>(&I))
		return L->isVolatile() && !L->isAtomic();
	if (const auto *S = llvm::dyn_cast<llvm::StoreInst>(&I))
		return S->isVolatile() && !S->isAtomic();
	return false;
}

/* Replace the atomic (or volatile) instruction I by a call to an external
 * function */
void lowerAtomic(llvm::Instruction *I)
{
	auto ord = [](llvm::AtomicOrdering o) {
		return o == llvm::AtomicOrdering::NotAtomic ? std::string("volatile")
							    : std::string(llvm::toIRString(o));
	};
	auto &ctx = I->getContext();
	const auto &DL = I->getModule()->getDataLayout();
	auto *voidTy = llvm::Type::getVoidTy(ctx);
	llvm::Value *result = nullptr;

	if (auto *L = llvm::dyn_cast<llvm::LoadInst>(I)) {
		auto *ty = accessType(L->getType(), DL);
		auto *ci = createExternalCall(
			L, "__alive_load_" + ord(L->getOrdering()) + "_" + typeTag(ty), ty,
			{L->getPointerOperand()});
		result = convert(ci, L->getType(), L);
	} else if (auto *S = llvm::dyn_cast<llvm::StoreInst>(I)) {
		auto *ty = accessType(S->getValueOperand()->getType(), DL);
		createExternalCall(S, "__alive_store_" + ord(S->getOrdering()) + "_" + typeTag(ty),
				   voidTy,
				   {convert(S->getValueOperand(), ty, S), S->getPointerOperand()});
	} else if (auto *A = llvm::dyn_cast<llvm::AtomicRMWInst>(I)) {
		auto *ty = accessType(A->getType(), DL);
		auto *ci = createExternalCall(
			A,
			"__alive_rmw_" +
				llvm::AtomicRMWInst::getOperationName(A->getOperation()).str() +
				"_" + ord(A->getOrdering()) + "_" + typeTag(ty),
			ty, {A->getPointerOperand(), convert(A->getValOperand(), ty, A)});
		result = convert(ci, A->getType(), A);
	} else if (auto *C = llvm::dyn_cast<llvm::AtomicCmpXchgInst>(I)) {
		auto *valTy = C->getCompareOperand()->getType();
		auto *ty = accessType(valTy, DL);
		auto *i1 = llvm::Type::getInt1Ty(ctx);
		auto *expected = convert(C->getCompareOperand(), ty, C);
		auto *ci = createExternalCall(
			C,
			std::string("__alive_cmpxchg") + (C->isWeak() ? "_weak_" : "_") +
				ord(C->getSuccessOrdering()) + "_" + ord(C->getFailureOrdering()) +
				"_" + typeTag(ty),
			llvm::StructType::get(ctx, {ty, i1}),
			{C->getPointerOperand(), expected, convert(C->getNewValOperand(), ty, C)});
		auto it = C->getIterator();
		auto *old = llvm::ExtractValueInst::Create(ci, {0}, "", it);
		auto *ok = llvm::ExtractValueInst::Create(ci, {1}, "", it);
		/* A cmpxchg that succeeds read the expected value; a strong one that
		 * fails did not */
		auto *same = new llvm::ICmpInst(it, llvm::CmpInst::ICMP_EQ, old, expected);
		llvm::Value *contract =
			C->isWeak() ? static_cast<llvm::Value *>(llvm::BinaryOperator::CreateOr(
					      llvm::BinaryOperator::CreateNot(ok, "", it), same, "",
					      it))
				    : new llvm::ICmpInst(it, llvm::CmpInst::ICMP_EQ, ok, same);
		auto assume = llvm::Intrinsic::getOrInsertDeclaration(C->getModule(),
								      llvm::Intrinsic::assume);
		llvm::CallInst::Create(assume, {contract}, "", it);
		llvm::Value *res = llvm::InsertValueInst::Create(
			llvm::PoisonValue::get(C->getType()), convert(old, valTy, C), {0}, "", it);
		result = llvm::InsertValueInst::Create(res, ok, {1}, "", it);
	} else if (auto *F = llvm::dyn_cast<llvm::FenceInst>(I)) {
		createExternalCall(F, "__alive_fence_" + ord(F->getOrdering()), voidTy, {});
	}
	if (result) {
		result->takeName(I);
		I->replaceAllUsesWith(result);
	}
	I->eraseFromParent();
}

/* Whether an internal function of code CODE orders nothing (an assume, a
 * marker, an annotation...) */
auto isPureCode(InternalFunctions code) -> bool
{
	auto codeI = static_cast<std::underlying_type_t<InternalFunctions>>(code);
	return
#define FIRST_PURE_FUNCTION(NUM) codeI >= NUM &&
#define LAST_PURE_FUNCTION(NUM) codeI <= NUM;
#include "passes/InternalFunction.def"
}

/*
 * What the functions GenMC interprets may touch: an access or a lock
 * operation its location, a fence, marker or assumption nothing (inaccessible
 * memory keeps them side effects a pass may not drop or reorder). Without
 * this, Alive2 lets every call write any global, and rejects a pass that
 * moves a plain access over an atomic one to another location.
 */
void restrictMemoryEffects(llvm::Module &mod)
{
	auto effects = llvm::MemoryEffects::argMemOnly(llvm::ModRefInfo::ModRef) |
		       llvm::MemoryEffects::inaccessibleMemOnly(llvm::ModRefInfo::ModRef);
	for (auto &F : mod) {
		if (!F.isDeclaration())
			continue;
		auto name = F.getName().str();
		auto restricted = name.starts_with("__alive_");
		if (isInternalFunction(name)) {
			auto code = internalFunNames.at(name);
			restricted = isPureCode(code) || isMutexCode(code) || isCondVarCode(code) ||
				     code == InternalFunctions::LkmmFence ||
				     code == InternalFunctions::SrcuReadLock ||
				     code == InternalFunctions::SrcuReadUnlock ||
				     code == InternalFunctions::SyncSrcu;
		}
		if (restricted)
			F.setMemoryEffects(effects);
	}
}

/*
 * The module as Alive2 can check it:
 *  - each atomic access, volatile access and fence becomes a call to an
 *    external function named after the operation, ordering and size (Alive2
 *    has no atomics; calls are side effects a refinement must keep, in
 *    number and order, as a transformation must keep such accesses), but
 *    for accesses to locals that do not escape, which become plain;
 *  - annotations become observable calls (exposeAnnotations);
 *  - the calls GenMC interprets touch only their location (restrictMemoryEffects);
 *  - GenMC's allocation functions return fresh memory (noalias);
 *  - debug information and alias-scope metadata go (Alive2 rejects them,
 *    and dropping them only drops facts).
 */
void makeCheckable(llvm::Module *clone, const AnnotationKinds &kinds, bool inlineCalls = false)
{
	llvm::StripDebugInfo(*clone);
	if (inlineCalls)
		inlineInternalCalls(*clone);
	/* LLVM 22's one-argument lifetime markers (the inliner adds some) do not
	 * parse in Alive2 v20's LLVM, and they only let a pass reuse a local's
	 * memory */
	for (auto &F : llvm::make_early_inc_range(*clone))
		if (F.getName().starts_with("llvm.lifetime.")) {
			for (auto *user : llvm::make_early_inc_range(F.users()))
				if (auto *ci = llvm::dyn_cast<llvm::CallInst>(user))
					ci->eraseFromParent();
			if (F.use_empty())
				F.eraseFromParent();
		}
	for (const auto *name :
	     {"__VERIFIER_malloc", "__VERIFIER_malloc_aligned", "__VERIFIER_palloc",
	      "__VERIFIER_hazptr_alloc", "__VERIFIER_calloc"})
		if (auto *fun = clone->getFunction(name))
			fun->addRetAttr(llvm::Attribute::NoAlias);
	exposeAnnotations(*clone, kinds);

	llvm::SmallVector<llvm::Instruction *, 16> worklist;
	for (auto &F : *clone)
		for (auto &I : llvm::instructions(F)) {
			I.setMetadata(llvm::LLVMContext::MD_noalias, nullptr);
			I.setMetadata(llvm::LLVMContext::MD_alias_scope, nullptr);
			if (isPrivateAccess(I)) {
				if (auto *L = llvm::dyn_cast<llvm::LoadInst>(&I))
					L->setAtomic(llvm::AtomicOrdering::NotAtomic);
				else
					llvm::cast<llvm::StoreInst>(&I)->setAtomic(
						llvm::AtomicOrdering::NotAtomic);
			} else if (I.isAtomic() || llvm::isa<llvm::FenceInst>(I) ||
				   isVolatileAccess(I)) {
				worklist.push_back(&I);
			} else if (auto *II = llvm::dyn_cast<llvm::IntrinsicInst>(&I);
				   II && II->getIntrinsicID() ==
						 llvm::Intrinsic::experimental_noalias_scope_decl) {
				worklist.push_back(&I);
			}
		}
	for (auto *I : worklist) {
		if (llvm::isa<llvm::IntrinsicInst>(I))
			I->eraseFromParent(); /* llvm.experimental.noalias.scope.decl */
		else
			lowerAtomic(I);
	}
	restrictMemoryEffects(*clone);
}

auto printCheckableModule(const llvm::Module &mod, const AnnotationKinds &kinds,
			  bool inlineCalls = false) -> std::string
{
	auto clone = llvm::CloneModule(mod);
	makeCheckable(&*clone, kinds, inlineCalls);
	return printModule(*clone);
}

/*
 * Dependencies, as GenMC's interpreter tracks them (lli/Runtime/Execution.cpp),
 * replayed in the module so that Alive2 can compare them across a pass.
 *
 * Sources. A dependency is on an event of the thread: a read (load, RMW,
 * cmpxchg), an allocation (alloca, __VERIFIER_malloc...), the thread join or
 * SRCU lock a value comes from, a spin or optional marker. Each source is a
 * bit of a mask (the smallest multiple of 64 bits that fits them, at most
 * MaxMaskBits), named by its source location (code a pass duplicates keeps
 * its bits), or by its value name without one; sources past the mask's
 * width, and anonymous ones, share the last bit. A function's arguments are
 * sources too: their dependencies come from the caller, which Alive2 does
 * not see, and a pass may not change what depends on them.
 *
 * Values. A source depends on itself. Any other value depends on what its
 * operands depend on (the interpreter: binary operators, comparisons, casts,
 * select on all three operands, GEP on the pointer and the indices, freeze,
 * extractvalue; insertvalue on the aggregate only, as the interpreter has
 * it); a PHI on the value it chooses, plus, under LKMM, the conditions of the
 * branches joining at its block. The result of a call to another function
 * has no dependencies here (the interpreter gives it the return value's; a
 * callee is checked on its own). GenMC does not carry data dependencies
 * through memory, the LKMM checker does (carry-dep = data ; rfi, and IMM has
 * rfi in ppo): a plain load of a global or local that the function stored to
 * before reads that store, and depends on what the stored value depended on,
 * so that a pass forwarding the stored value keeps the dependencies.
 *
 * Control. A conditional branch, switch or indirect branch makes what runs
 * after it depend on its condition: to the end of the function without
 * scoping (IMM), and under LKMM (scoped) until its join, the block's
 * immediate post-dominator, is entered, after which only the join's PHIs
 * carry the dependency; a branch without a join (an early return, an endless
 * loop) is not scoped. Permanent control dependencies also come from the
 * argument of an assume, from a spin or optional marker (on the marker's
 * event) and, under LKMM, from a cmpxchg that acquires on success only (on
 * the cmpxchg), as the interpreter adds them.
 *
 * Obligations. Before each event that carries dependencies, the module calls
 * __alive_deps(addr, data, ctrl): atomic loads, stores, RMWs and cmpxchgs
 * (addr: the pointer; data: the value stored, the RMW operand, the cmpxchg
 * new value), plain stores to memory another thread may see, mutex and
 * condition-variable calls and SRCU locks (addr: the first argument; the
 * SRCU unlock's data: the second), thread creation (data: the argument
 * passed), fences, and the other internal calls that make events (ctrl
 * only), and calls to the module's own functions (ctrl only: the callee's
 * events run under the caller's open branches). The masks are passed as
 * vectors of bits, and a pass that changes one is reported by alive-tv in
 * the direction the source module allows (a poison lane is refined by
 * anything): with the source's set bits poison, a target bit the source
 * lacks is an added dependency (the "+deps-add" pair); with its clear bits
 * poison, a missing one is a dropped dependency ("+deps-drop"). A
 * dependency that changes kind (a branch that becomes a select: control to
 * data) fails both.
 *
 * Not modelled: the cmpxchg's expected value and the addr;po dependencies
 * (tracked, but no checker reads them); the prefix-only control dependencies
 * of LKMM (pctrl: what a branch's accesses can order after their join, which
 * depends on the branch's locations); the dependencies a callee's events get
 * from an indirect call's function pointer; accesses to thread-local storage;
 * a plain load's own event when it reads a store of the function (its
 * dependencies are the stored value's, so that forwarding is no change).
 */
constexpr unsigned MaxMaskBits = 256;
struct SourceBits {
	std::map<std::string, unsigned> bits;
	unsigned width;
};

/* What the source module's masks tolerate, so that alive-tv reports a change
 * in one direction only */
enum class DepCheck : std::uint8_t { Added, Dropped };

auto internalCode(const llvm::Instruction &I) -> std::optional<InternalFunctions>
{
	auto *ci = llvm::dyn_cast<llvm::CallInst>(&I);
	if (!ci)
		return std::nullopt;
	auto name = getCalledFunOrStripValName(*ci);
	if (!isInternalFunction(name))
		return std::nullopt;
	return internalFunNames.at(name);
}

/* Calls whose result depends on their own event */
auto isEventValuedCode(InternalFunctions code) -> bool
{
	return isAllocCode(code) || code == InternalFunctions::ThreadJoin ||
	       code == InternalFunctions::SrcuReadLock;
}

/* Markers that put a control dependency on their own event */
auto isCtrlMarkerCode(InternalFunctions code) -> bool
{
	using IF = InternalFunctions;
	return code == IF::OptBegin || code == IF::LoopBegin || code == IF::SpinStart ||
	       code == IF::AwaitStart || code == IF::FaiZNESpinEnd || code == IF::LockZNESpinEnd;
}

auto isSource(const llvm::Instruction &I) -> bool
{
	if (llvm::isa<llvm::LoadInst, llvm::AtomicRMWInst, llvm::AtomicCmpXchgInst,
		      llvm::AllocaInst>(I))
		return true;
	auto code = internalCode(I);
	return code && (isEventValuedCode(*code) || isCtrlMarkerCode(*code));
}

/* A source's name across a pass: its source location, with where it was
 * inlined, or its value name */
auto sourceName(const llvm::Instruction &I) -> std::string
{
	std::string s;
	for (const auto *loc = I.getDebugLoc().get(); loc; loc = loc->getInlinedAt())
		s += std::to_string(loc->getLine()) + ":" + std::to_string(loc->getColumn()) + "/";
	if (s.empty() && I.hasName())
		s = "%" + I.getName().str();
	return s;
}

auto sourceName(const llvm::Argument &A) -> std::string
{
	return "arg:" + A.getParent()->getName().str() + ":" + std::to_string(A.getArgNo());
}

/* Bits for the sources of the modules: one per name, the last for the
 * anonymous ones and those past the mask's width */
auto assignSourceBits(std::initializer_list<const llvm::Module *> mods) -> SourceBits
{
	SourceBits sources;
	auto &bits = sources.bits;
	for (const auto *mod : mods)
		for (const auto &F : *mod) {
			for (const auto &A : F.args())
				bits.try_emplace(sourceName(A), 0);
			for (const auto &I : llvm::instructions(F))
				if (isSource(I))
					bits.try_emplace(sourceName(I), 0);
		}
	auto named = bits.size() - bits.count("");
	sources.width = std::min(MaxMaskBits, 64 * static_cast<unsigned>((named + 1 + 63) / 64));
	auto i = 0U;
	for (auto &[name, bit] : bits)
		bit = name.empty() ? sources.width - 1 : std::min(i++, sources.width - 1);
	return sources;
}

/* The condition a terminator branches on, if any */
auto branchCondition(llvm::Instruction &I) -> llvm::Value *
{
	if (auto *br = llvm::dyn_cast<llvm::BranchInst>(&I))
		return br->isConditional() ? br->getCondition() : nullptr;
	if (auto *sw = llvm::dyn_cast<llvm::SwitchInst>(&I))
		return sw->getCondition();
	if (auto *ib = llvm::dyn_cast<llvm::IndirectBrInst>(&I))
		return ib->getAddress();
	return nullptr;
}

void instrumentDeps(llvm::Function &F, const SourceBits &sources, bool scoped,
		    std::optional<DepCheck> check)
{
	auto &ctx = F.getContext();
	auto width = sources.width;
	auto *maskTy = llvm::IntegerType::get(ctx, width);
	auto *i1 = llvm::Type::getInt1Ty(ctx);
	auto *lanesTy = llvm::FixedVectorType::get(i1, width);
	auto *zero = llvm::ConstantInt::get(maskTy, 0);
	auto depsFun = F.getParent()->getOrInsertFunction(
		"__alive_deps", llvm::FunctionType::get(llvm::Type::getVoidTy(ctx),
							{lanesTy, lanesTy, lanesTy}, false));
	auto bitOf = [&](const std::string &name) -> llvm::Constant * {
		auto it = sources.bits.find(name);
		auto bit = it != sources.bits.end() ? it->second : width - 1;
		return llvm::ConstantInt::get(maskTy, llvm::APInt::getOneBitSet(width, bit));
	};

	/* The original blocks and instructions, before anything goes in */
	llvm::ReversePostOrderTraversal<llvm::Function *> RPO(&F);
	const std::vector<llvm::BasicBlock *> blocks(RPO.begin(), RPO.end());
	llvm::DenseMap<llvm::BasicBlock *, llvm::SmallVector<llvm::Instruction *, 16>> orig;
	llvm::SmallVector<llvm::PHINode *, 16> origPhis;
	for (auto *BB : blocks)
		for (auto &I : *BB) {
			if (auto *phi = llvm::dyn_cast<llvm::PHINode>(&I))
				origPhis.push_back(phi);
			else
				orig[BB].push_back(&I);
		}

	/* The state slots, promoted to registers at the end */
	auto entryPos = F.getEntryBlock().getFirstInsertionPt();
	std::vector<llvm::AllocaInst *> slots;
	auto newSlot = [&](llvm::Type *ty, llvm::Constant *init, const char *name) {
		auto *slot = new llvm::AllocaInst(ty, 0, name, entryPos);
		new llvm::StoreInst(init, slot, entryPos);
		slots.push_back(slot);
		return slot;
	};
	auto load = [&](llvm::AllocaInst *slot, llvm::BasicBlock::iterator pos) -> llvm::Value * {
		return new llvm::LoadInst(slot->getAllocatedType(), slot, "", pos);
	};
	auto orInto = [&](llvm::AllocaInst *slot, llvm::Value *v, llvm::BasicBlock::iterator pos) {
		if (v == zero)
			return;
		new llvm::StoreInst(llvm::BinaryOperator::CreateOr(load(slot, pos), v, "", pos), slot,
				    pos);
	};

	/* The dependencies that never end, and those open until a join */
	auto *perm = newSlot(maskTy, zero, "deps.perm");
	llvm::PostDominatorTree PDT(F);
	auto joinOf = [&](llvm::BasicBlock *BB) -> llvm::BasicBlock * {
		auto *node = PDT.getNode(BB);
		auto *idom = node ? node->getIDom() : nullptr;
		return idom ? idom->getBlock() : nullptr;
	};
	llvm::MapVector<llvm::BasicBlock *, llvm::AllocaInst *> joinSlots;
	if (scoped)
		for (auto *BB : blocks)
			if (branchCondition(*BB->getTerminator()))
				if (auto *join = joinOf(BB); join && !joinSlots.contains(join))
					joinSlots[join] = newSlot(maskTy, zero, "deps.join");
	auto ctrlAt = [&](llvm::BasicBlock::iterator pos) -> llvm::Value * {
		auto *acc = load(perm, pos);
		for (auto &[join, slot] : joinSlots)
			acc = llvm::BinaryOperator::CreateOr(acc, load(slot, pos), "", pos);
		return acc;
	};

	/* What each value depends on */
	llvm::DenseMap<llvm::Value *, llvm::Value *> shadow;
	for (auto &A : F.args())
		shadow[&A] = bitOf(sourceName(A));
	auto shadowOf = [&](llvm::Value *v) -> llvm::Value * {
		auto it = shadow.find(v);
		return it != shadow.end() ? it->second : zero;
	};
	auto orAll = [&](llvm::ArrayRef<llvm::Value *> vs,
			 llvm::BasicBlock::iterator pos) -> llvm::Value * {
		llvm::Value *acc = zero;
		for (auto *v : vs)
			if (v != zero)
				acc = acc == zero ? v : llvm::BinaryOperator::CreateOr(acc, v, "", pos);
		return acc;
	};
	/* Shadow PHIs first: a PHI's operands may come later */
	llvm::SmallVector<std::pair<llvm::PHINode *, llvm::PHINode *>, 16> phis;
	for (auto *phi : origPhis) {
		auto *sp = llvm::PHINode::Create(maskTy, phi->getNumIncomingValues(), "deps.phi",
						 phi->getParent()->begin());
		phis.push_back({phi, sp});
		shadow[phi] = sp;
	}

	/* Per global or local the function stores to plainly: the dependencies of
	 * the last value stored, and whether there was one (carry-dep) */
	struct Carried {
		llvm::AllocaInst *mask;
		llvm::AllocaInst *set;
	};
	llvm::DenseMap<const llvm::Value *, Carried> carried;
	auto objectOf = [](llvm::Value *ptr) -> const llvm::Value * {
		const auto *obj = llvm::getUnderlyingObject(ptr);
		return llvm::isa<llvm::GlobalVariable, llvm::AllocaInst>(obj) ? obj : nullptr;
	};
	for (auto *BB : blocks)
		for (auto *I : orig[BB])
			if (auto *S = llvm::dyn_cast<llvm::StoreInst>(I); S && !S->isAtomic())
				if (const auto *obj = objectOf(S->getPointerOperand());
				    obj && !carried.contains(obj))
					carried[obj] = {
						newSlot(maskTy, zero, "deps.carried"),
						newSlot(i1, llvm::ConstantInt::getFalse(ctx), "deps.set")};

	/* A mask as lanes; in the source module, with the lanes the check
	 * leaves free poison */
	auto encode = [&](llvm::Value *m, llvm::BasicBlock::iterator pos) -> llvm::Value * {
		llvm::Value *lanes = new llvm::BitCastInst(m, lanesTy, "", pos);
		if (!check)
			return lanes;
		auto *poison = llvm::PoisonValue::get(lanesTy);
		return *check == DepCheck::Added
			       ? llvm::SelectInst::Create(lanes, poison,
							  llvm::Constant::getNullValue(lanesTy), "", pos)
			       : llvm::SelectInst::Create(lanes, llvm::Constant::getAllOnesValue(lanesTy),
							  poison, "", pos);
	};
	auto obligation = [&](llvm::Value *addr, llvm::Value *data, llvm::BasicBlock::iterator pos) {
		llvm::CallInst::Create(depsFun,
				       {encode(addr, pos), encode(data, pos), encode(ctrlAt(pos), pos)},
				       "", pos);
	};
	auto arg = [&](llvm::Instruction &I, unsigned i) -> llvm::Value * {
		auto &cb = llvm::cast<llvm::CallBase>(I);
		return i < cb.arg_size() ? shadowOf(cb.getArgOperand(i)) : zero;
	};

	for (auto *BB : blocks) {
		/* Entering a join ends its branches' dependencies; its PHIs carry them */
		auto pos = BB->getFirstInsertionPt();
		if (auto it = joinSlots.find(BB); it != joinSlots.end()) {
			auto *joined = load(it->second, pos);
			new llvm::StoreInst(zero, it->second, pos);
			for (auto &[phi, sp] : phis)
				if (phi->getParent() == BB)
					shadow[phi] = llvm::BinaryOperator::CreateOr(sp, joined, "", pos);
		}

		for (auto *IP : orig[BB]) {
			auto &I = *IP;
			auto it = I.getIterator();
			auto next = std::next(it);

			/* The event's obligation */
			if (auto *L = llvm::dyn_cast<llvm::LoadInst>(&I)) {
				if (L->isAtomic() && !isPrivateAccess(I))
					obligation(shadowOf(L->getPointerOperand()), zero, it);
			} else if (auto *S = llvm::dyn_cast<llvm::StoreInst>(&I)) {
				if (!isPrivateAccess(I))
					obligation(shadowOf(S->getPointerOperand()),
						   shadowOf(S->getValueOperand()), it);
			} else if (auto *A = llvm::dyn_cast<llvm::AtomicRMWInst>(&I)) {
				obligation(shadowOf(A->getPointerOperand()), shadowOf(A->getValOperand()),
					   it);
			} else if (auto *C = llvm::dyn_cast<llvm::AtomicCmpXchgInst>(&I)) {
				obligation(shadowOf(C->getPointerOperand()),
					   shadowOf(C->getNewValOperand()), it);
			} else if (llvm::isa<llvm::FenceInst>(I)) {
				obligation(zero, zero, it);
			} else if (auto code = internalCode(I)) {
				using IF = InternalFunctions;
				if (isMutexCode(*code) || isCondVarCode(*code) || *code == IF::SrcuReadLock)
					obligation(arg(I, 0), zero, it);
				else if (*code == IF::SrcuReadUnlock)
					obligation(arg(I, 0), arg(I, 1), it);
				else if (*code == IF::ThreadCreate || *code == IF::ThreadCreateSymmetric)
					obligation(zero, arg(I, 2), it);
				else if (*code == IF::Assume)
					orInto(perm, arg(I, 0), it);
				else if (isCtrlMarkerCode(*code))
					orInto(perm, bitOf(sourceName(I)), it);
				else if (!isPureCode(*code))
					obligation(zero, zero, it);
			} else if (auto *ci = llvm::dyn_cast<llvm::CallInst>(&I)) {
				auto *callee = ci->getCalledFunction();
				if (callee && !callee->isDeclaration())
					obligation(zero, zero, it);
			}

			/* A plain store leaves its value's dependencies for the loads after it */
			if (auto *S = llvm::dyn_cast<llvm::StoreInst>(&I); S && !S->isAtomic())
				if (const auto *obj = objectOf(S->getPointerOperand())) {
					auto &c = carried[obj];
					new llvm::StoreInst(shadowOf(S->getValueOperand()), c.mask, it);
					new llvm::StoreInst(llvm::ConstantInt::getTrue(ctx), c.set, it);
				}

			/* The value's dependencies */
			auto *L = llvm::dyn_cast<llvm::LoadInst>(&I);
			const auto *obj = L && !L->isAtomic() ? objectOf(L->getPointerOperand()) : nullptr;
			if (auto cit = obj ? carried.find(obj) : carried.end(); cit != carried.end()) {
				shadow[&I] = llvm::SelectInst::Create(load(cit->second.set, next),
								      load(cit->second.mask, next),
								      bitOf(sourceName(I)), "", next);
			} else if (isSource(I)) {
				shadow[&I] = bitOf(sourceName(I));
				/* A cmpxchg acquiring on success only: what follows depends on it */
				if (auto *C = llvm::dyn_cast<llvm::AtomicCmpXchgInst>(&I);
				    C && scoped && C->getSuccessOrdering() != C->getFailureOrdering())
					orInto(perm, bitOf(sourceName(I)), next);
			} else if (auto *IV = llvm::dyn_cast<llvm::InsertValueInst>(&I)) {
				shadow[&I] = shadowOf(IV->getAggregateOperand());
			} else if (!I.getType()->isVoidTy() && !llvm::isa<llvm::CallBase>(I)) {
				llvm::SmallVector<llvm::Value *, 4> ops;
				for (auto &op : I.operands())
					ops.push_back(shadowOf(op.get()));
				shadow[&I] = orAll(ops, it);
			}

			/* A branch opens a scope until its join, or never ends */
			if (auto *cond = branchCondition(I)) {
				auto *join = scoped ? joinOf(BB) : nullptr;
				orInto(join ? joinSlots[join] : perm, shadowOf(cond), it);
			}
		}
	}

	for (auto &[phi, sp] : phis)
		for (auto i = 0U; i < phi->getNumIncomingValues(); i++)
			sp->addIncoming(shadowOf(phi->getIncomingValue(i)), phi->getIncomingBlock(i));

	llvm::DominatorTree DT(F);
	llvm::PromoteMemToReg(slots, DT);
}

auto printDepsModule(const llvm::Module &mod, const SourceBits &sources, bool scoped,
		     const AnnotationKinds &kinds, std::optional<DepCheck> check) -> std::string
{
	auto clone = llvm::CloneModule(mod);
	for (auto &F : *clone)
		if (!F.isDeclaration())
			instrumentDeps(F, sources, scoped, check);
	makeCheckable(&*clone, kinds);
	return printModule(*clone);
}

/* The passes whose dependencies are compared: those that keep the module's
 * memory layout and stay within functions (promoting locals to registers
 * and inlining change what GenMC's dependencies are on; spin-assume and
 * assume propagation add assumptions on purpose, checked elsewhere) */
auto checksDeps(llvm::StringRef passID) -> bool
{
	static const std::set<llvm::StringRef> skipped = {
		"SROAPass",	       "PromotePass",	       "PromoteMemIntrinsicPass",
		"IntrinsicLoweringPass", "FunctionInlinerPass",   "DeadArgumentEliminationPass",
		"DeclareInternalsPass",	 "DefineLibcFunsPass",	   "MDataCollectionPass",
		"RustPrepPass",		 "SpinAssumePass",	   "PropagateAssumesPass"};
	return !skipped.contains(passID);
}

/*
 * The module as GenMC runs it, for the obligation modules: GenMC's values
 * are never poison (arithmetic wraps, a plain load reads some value), while
 * Alive2 lets nsw/nuw/exact results and plain loads be poison, which an
 * obligation's assumption would turn into UB. Freezing the loads and
 * dropping the flags only makes the target more defined.
 */
void makeDefined(llvm::Module &mod)
{
	for (auto &F : mod) {
		llvm::SmallVector<llvm::LoadInst *, 16> loads;
		for (auto &I : llvm::instructions(F)) {
			I.dropPoisonGeneratingFlags();
			if (auto *L = llvm::dyn_cast<llvm::LoadInst>(&I); L && !L->isAtomic())
				loads.push_back(L);
		}
		for (auto *L : loads) {
			auto *fr = new llvm::FreezeInst(L, L->getName() + ".fr",
							std::next(L->getIterator()));
			L->replaceUsesWithIf(fr, [&](llvm::Use &u) { return u.getUser() != fr; });
		}
	}
}

/* Remove the calls to the internal function NAME */
void eraseCallsTo(llvm::Module &mod, llvm::StringRef name)
{
	auto *fun = mod.getFunction(name);
	if (!fun)
		return;
	for (auto *user : llvm::make_early_inc_range(fun->users()))
		if (auto *ci = llvm::dyn_cast<llvm::CallInst>(user))
			ci->eraseFromParent();
}

/*
 * The module after SpinAssumePass, with each blocking marker replaced by
 * what it claims, as an assumption (Alive2: a failed llvm.assume is UB, so
 * the module before the pass refines this one iff no claim can fail):
 *
 *  - __VERIFIER_assume_internal(c, Spinloop) at the end of a latch blocks
 *    unless c: if !c, the latch goes back to the header and the next
 *    iteration repeats this one (no header PHI changes);
 *  - __VERIFIER_{lock,fai}ZNE_spin_end(c) blocks if c, which claims that the
 *    iteration goes back to the header: a flag, cleared at the header and set
 *    by the spin end, may not be set at a loop exit.
 *
 * __VERIFIER_spin_start() and __VERIFIER_loop_begin() never block and go.
 */
auto printSpinObligationModule(const llvm::Module &mod, const AnnotationKinds &kinds) -> std::string
{
	auto clone = llvm::CloneModule(mod);
	makeDefined(*clone);
	auto &ctx = clone->getContext();
	auto *i1 = llvm::Type::getInt1Ty(ctx);
	auto assume = llvm::Intrinsic::getOrInsertDeclaration(&*clone, llvm::Intrinsic::assume);
	auto addAssume = [&](llvm::Value *cond, llvm::BasicBlock::iterator pos) {
		llvm::CallInst::Create(assume, {cond}, "", pos);
	};

	for (auto &F : *clone) {
		if (F.isDeclaration())
			continue;
		llvm::DominatorTree DT(F);
		llvm::LoopInfo LI(DT);
		llvm::SmallVector<llvm::CallInst *, 4> spinEnds, zneEnds;
		for (auto &I : llvm::instructions(F)) {
			auto *ci = llvm::dyn_cast<llvm::CallInst>(&I);
			auto *callee = ci ? ci->getCalledFunction() : nullptr;
			if (!callee)
				continue;
			auto name = callee->getName();
			auto *kind =
				name == "__VERIFIER_assume_internal"
					? llvm::dyn_cast<llvm::ConstantInt>(ci->getArgOperand(1))
					: nullptr;
			if (kind && kind->getZExtValue() ==
					    static_cast<std::uint64_t>(AssumeType::Spinloop))
				spinEnds.push_back(ci);
			else if (name == "__VERIFIER_lockZNE_spin_end" ||
				 name == "__VERIFIER_faiZNE_spin_end")
				zneEnds.push_back(ci);
		}

		for (auto *ci : spinEnds) {
			auto *latch = ci->getParent();
			auto *loop = LI.getLoopFor(latch);
			auto *br = llvm::dyn_cast<llvm::BranchInst>(latch->getTerminator());
			ERROR_ON(!loop || !br, "Spin-loop assume outside a loop latch in {}\n",
				 F.getName().str());
			auto *header = loop->getHeader();
			auto pos = ci->getIterator();
			/* The latch goes back to the header ... */
			llvm::Value *back =
				llvm::ConstantInt::getBool(ctx, br->getSuccessor(0) == header);
			if (br->isConditional())
				back = llvm::SelectInst::Create(
					br->getCondition(),
					llvm::ConstantInt::getBool(ctx,
								   br->getSuccessor(0) == header),
					llvm::ConstantInt::getBool(ctx,
								   br->getSuccessor(1) == header),
					"", pos);
			/* ... and no header PHI changes */
			llvm::Value *repeats = back;
			for (auto &phi : header->phis()) {
				auto *next = phi.getIncomingValueForBlock(latch);
				if (next == &phi)
					continue;
				auto *eq =
					phi.getType()->isFloatingPointTy()
						? static_cast<llvm::Value *>(new llvm::FCmpInst(
							  pos, llvm::CmpInst::FCMP_OEQ, next, &phi))
						: new llvm::ICmpInst(pos, llvm::CmpInst::ICMP_EQ,
								     next, &phi);
				repeats = llvm::BinaryOperator::CreateAnd(repeats, eq, "", pos);
			}
			/* c || repeats */
			addAssume(llvm::BinaryOperator::CreateOr(ci->getArgOperand(0), repeats, "",
								 pos),
				  pos);
			ci->eraseFromParent();
		}

		for (auto *ci : zneEnds) {
			auto *loop = LI.getLoopFor(ci->getParent());
			ERROR_ON(!loop, "ZNE spin end outside a loop in {}\n", F.getName().str());
			auto *flag = new llvm::AllocaInst(i1, 0, "zne.flag",
							  F.getEntryBlock().getFirstInsertionPt());
			new llvm::StoreInst(llvm::ConstantInt::getFalse(ctx), flag,
					    std::next(flag->getIterator()));
			new llvm::StoreInst(llvm::ConstantInt::getFalse(ctx), flag,
					    loop->getHeader()->getFirstInsertionPt());
			auto pos = ci->getIterator();
			auto *old = new llvm::LoadInst(i1, flag, "", pos);
			new llvm::StoreInst(
				llvm::BinaryOperator::CreateOr(old, ci->getArgOperand(0), "", pos),
				flag, pos);
			llvm::SmallVector<llvm::BasicBlock *, 4> exits;
			loop->getExitBlocks(exits);
			for (auto *exit : exits) {
				auto epos = exit->getFirstInsertionPt();
				auto *set = new llvm::LoadInst(i1, flag, "", epos);
				addAssume(llvm::BinaryOperator::CreateNot(set, "", epos), epos);
			}
			ci->eraseFromParent();
		}
	}
	eraseCallsTo(*clone, "__VERIFIER_spin_start");
	eraseCallsTo(*clone, "__VERIFIER_loop_begin");
	makeCheckable(&*clone, kinds);
	return printModule(*clone);
}

/* The value the program uses for V: the freeze makeDefined put after a plain load */
auto frozen(llvm::Value *v) -> llvm::Value *
{
	if (auto *L = llvm::dyn_cast<llvm::LoadInst>(v); L && !L->isAtomic())
		for (auto *user : L->users())
			if (llvm::isa<llvm::FreezeInst>(user))
				return user;
	return v;
}

using AnnotExpr = SExpr<llvm::Value *>;

/* E as an integer of E's width, built before POS; nullptr if E uses a value
 * that is not available at POS or a kind this does not build */
auto buildAnnotExpr(const AnnotExpr *e, llvm::Instruction *pos, llvm::DominatorTree &DT,
		    llvm::ValueToValueMapTy &VMap) -> llvm::Value *
{
	using Kind = AnnotExpr::Kind;
	auto &ctx = pos->getContext();
	auto *ty = llvm::IntegerType::get(ctx, e->getWidth());
	auto it = pos->getIterator();
	auto resize = [&](llvm::Value *v, llvm::Type *to) -> llvm::Value * {
		if (v->getType() == to)
			return v;
		if (v->getType()->isPointerTy())
			v = new llvm::PtrToIntInst(v, llvm::IntegerType::get(ctx, 64), "", it);
		if (!v->getType()->isIntegerTy())
			return nullptr;
		return llvm::CastInst::CreateIntegerCast(v, to, false, "", it);
	};
	auto kid = [&](unsigned i) { return buildAnnotExpr(e->getKid(i), pos, DT, VMap); };

	if (auto *c = llvm::dyn_cast<ConcreteExpr<llvm::Value *>>(e))
		return llvm::ConstantInt::get(ty, c->getValue().get());
	if (auto *r = llvm::dyn_cast<RegisterExpr<llvm::Value *>>(e)) {
		llvm::Value *v = r->getRegister();
		if (auto vit = VMap.find(v); vit != VMap.end())
			v = vit->second;
		v = frozen(v);
		auto *inst = llvm::dyn_cast<llvm::Instruction>(v);
		if (inst && !DT.dominates(inst, pos))
			return nullptr;
		return resize(v, ty);
	}
	switch (e->getKind()) {
	case Kind::Select: {
		auto *c = kid(0), *t = kid(1), *f = kid(2);
		if (!c || !t || !f)
			return nullptr;
		c = new llvm::ICmpInst(it, llvm::CmpInst::ICMP_NE, c,
				       llvm::ConstantInt::get(c->getType(), 0));
		return llvm::SelectInst::Create(c, t, f, "", it);
	}
	case Kind::Conjunction:
	case Kind::Disjunction: {
		llvm::Value *acc = nullptr;
		for (auto i = 0U; i < e->getNumKids(); i++) {
			auto *k = kid(i);
			if (!k || !(k = resize(k, ty)))
				return nullptr;
			acc = !acc ? k
				   : llvm::BinaryOperator::Create(e->getKind() == Kind::Conjunction
									  ? llvm::Instruction::And
									  : llvm::Instruction::Or,
								  acc, k, "", it);
		}
		return acc ? acc
			   : llvm::ConstantInt::get(ty, e->getKind() == Kind::Conjunction ? 1 : 0);
	}
	case Kind::Not: {
		auto *k = kid(0);
		if (!k)
			return nullptr;
		auto *isZero = new llvm::ICmpInst(it, llvm::CmpInst::ICMP_EQ, k,
						  llvm::ConstantInt::get(k->getType(), 0));
		return resize(isZero, ty);
	}
	case Kind::ZExt:
	case Kind::SExt:
	case Kind::Trunc: {
		auto *k = kid(0);
		if (!k)
			return nullptr;
		return llvm::CastInst::CreateIntegerCast(k, ty, e->getKind() == Kind::SExt, "", it);
	}
	default:
		break;
	}
	if (e->getKind() >= Kind::BinaryKindFirst && e->getKind() <= Kind::BinaryKindLast) {
		auto *a = kid(0), *b = kid(1);
		if (!a || !b)
			return nullptr;
		if (e->getKind() > Kind::CmpKindFirst && e->getKind() < Kind::CmpKindLast) {
			static const std::map<Kind, llvm::CmpInst::Predicate> preds = {
				{Kind::Eq, llvm::CmpInst::ICMP_EQ},
				{Kind::Ne, llvm::CmpInst::ICMP_NE},
				{Kind::Ult, llvm::CmpInst::ICMP_ULT},
				{Kind::Ule, llvm::CmpInst::ICMP_ULE},
				{Kind::Ugt, llvm::CmpInst::ICMP_UGT},
				{Kind::Uge, llvm::CmpInst::ICMP_UGE},
				{Kind::Slt, llvm::CmpInst::ICMP_SLT},
				{Kind::Sle, llvm::CmpInst::ICMP_SLE},
				{Kind::Sgt, llvm::CmpInst::ICMP_SGT},
				{Kind::Sge, llvm::CmpInst::ICMP_SGE}};
			if (a->getType() != b->getType() && !(b = resize(b, a->getType())))
				return nullptr;
			return resize(new llvm::ICmpInst(it, preds.at(e->getKind()), a, b), ty);
		}
		static const std::map<Kind, llvm::Instruction::BinaryOps> ops = {
			{Kind::Add, llvm::Instruction::Add},
			{Kind::Sub, llvm::Instruction::Sub},
			{Kind::Mul, llvm::Instruction::Mul},
			{Kind::UDiv, llvm::Instruction::UDiv},
			{Kind::SDiv, llvm::Instruction::SDiv},
			{Kind::URem, llvm::Instruction::URem},
			{Kind::SRem, llvm::Instruction::SRem},
			{Kind::And, llvm::Instruction::And},
			{Kind::Or, llvm::Instruction::Or},
			{Kind::Xor, llvm::Instruction::Xor},
			{Kind::Shl, llvm::Instruction::Shl},
			{Kind::LShr, llvm::Instruction::LShr},
			{Kind::AShr, llvm::Instruction::AShr}};
		if (!(a = resize(a, ty)) || !(b = resize(b, ty)))
			return nullptr;
		auto k = e->getKind();
		if (k == Kind::UDiv || k == Kind::SDiv || k == Kind::URem || k == Kind::SRem) {
			/* GenMC's evaluator: 0 for a zero divisor or a signed overflow */
			auto *zero = llvm::ConstantInt::get(ty, 0);
			llvm::Value *bad = new llvm::ICmpInst(it, llvm::CmpInst::ICMP_EQ, b, zero);
			if (k == Kind::SDiv || k == Kind::SRem) {
				auto *minA = new llvm::ICmpInst(
					it, llvm::CmpInst::ICMP_EQ, a,
					llvm::ConstantInt::get(
						ty, llvm::APInt::getSignedMinValue(e->getWidth())));
				auto *minus1 =
					new llvm::ICmpInst(it, llvm::CmpInst::ICMP_EQ, b,
							   llvm::ConstantInt::getAllOnesValue(ty));
				bad = llvm::BinaryOperator::CreateOr(
					bad, llvm::BinaryOperator::CreateAnd(minA, minus1, "", it),
					"", it);
			}
			auto *safeB = llvm::SelectInst::Create(bad, llvm::ConstantInt::get(ty, 1),
							       b, "", it);
			auto *q = llvm::BinaryOperator::Create(ops.at(k), a, safeB, "", it);
			return llvm::SelectInst::Create(bad, zero, q, "", it);
		}
		return llvm::BinaryOperator::Create(ops.at(k), a, b, "", it);
	}
	return nullptr; /* Concat, Extract */
}

/* Whether I is a side effect a blocked read would lose */
auto isSideEffect(const llvm::Instruction &I) -> bool
{
	if (auto *S = llvm::dyn_cast<llvm::StoreInst>(&I))
		return !llvm::isa<llvm::AllocaInst>(S->getPointerOperand()->stripPointerCasts());
	if (llvm::isa<llvm::AtomicRMWInst, llvm::AtomicCmpXchgInst, llvm::FenceInst>(I))
		return true;
	auto *ci = llvm::dyn_cast<llvm::CallInst>(&I);
	if (!ci || llvm::isa<llvm::IntrinsicInst>(ci))
		return false;
	auto *callee = ci->getCalledFunction();
	auto name = callee ? callee->getName() : llvm::StringRef();
	return !(name == "__VERIFIER_assume_internal" || name == "__VERIFIER_spin_start" ||
		 name == "__VERIFIER_loop_begin" || name.starts_with("__VERIFIER_annotate"));
}

/*
 * The final module with what each load annotation claims, as assumptions.
 * GenMC does not explore a read for which the annotation of its load is
 * false: such a read must fail the next assume of the annotation's kind,
 * with no side effect before it. A flag per annotated load, set after the
 * load when the annotation is false, may not be set at a side effect, at a
 * return, or at an assume of that kind whose condition holds; the assume
 * clears it. Annotations that cannot be built at their load (a value defined
 * after it) are returned in UNBUILT.
 */
auto printAnnotationObligationModule(
	const llvm::Module &mod, const AnnotationInfo<llvm::Instruction *, llvm::Value *> &annots,
	const AnnotationKinds &kinds, std::vector<std::string> &unbuilt) -> std::string
{
	llvm::ValueToValueMapTy VMap;
	auto clone = llvm::CloneModule(mod, VMap);
	makeDefined(*clone);
	auto &ctx = clone->getContext();
	auto *i1 = llvm::Type::getInt1Ty(ctx);
	auto assume = llvm::Intrinsic::getOrInsertDeclaration(&*clone, llvm::Intrinsic::assume);

	std::map<llvm::Function *, std::vector<std::pair<llvm::AllocaInst *, AssumeType>>> flags;
	std::map<llvm::Function *, std::unique_ptr<llvm::DominatorTree>> DTs;
	for (const auto &[load, annot] : annots.annotMap) {
		auto *L = llvm::cast<llvm::Instruction>(VMap[load]);
		auto *F = L->getFunction();
		auto &DT = DTs[F];
		if (!DT)
			DT = std::make_unique<llvm::DominatorTree>(*F);
		/* After the load's freeze (makeDefined), which the program uses */
		auto *pos = llvm::cast<llvm::Instruction>(frozen(L))->getNextNode();
		auto *holds = buildAnnotExpr(&*annot.second, pos, *DT, VMap);
		if (!holds) {
			std::string s;
			llvm::raw_string_ostream os(s);
			os << F->getName() << ": " << *L;
			unbuilt.push_back(s);
			continue;
		}
		auto *flag = new llvm::AllocaInst(i1, 0, "annot.flag",
						  F->getEntryBlock().getFirstInsertionPt());
		new llvm::StoreInst(llvm::ConstantInt::getFalse(ctx), flag,
				    std::next(flag->getIterator()));
		auto *fails = new llvm::ICmpInst(pos->getIterator(), llvm::CmpInst::ICMP_EQ, holds,
						 llvm::ConstantInt::get(holds->getType(), 0));
		new llvm::StoreInst(fails, flag, pos->getIterator());
		flags[F].push_back({flag, annot.first});
	}

	for (auto &[F, fs] : flags) {
		llvm::SmallVector<llvm::Instruction *, 16> points;
		for (auto &I : llvm::instructions(*F))
			if (isSideEffect(I) || llvm::isa<llvm::ReturnInst>(I) ||
			    (llvm::isa<llvm::CallInst>(I) &&
			     llvm::cast<llvm::CallInst>(I).getCalledFunction() &&
			     llvm::cast<llvm::CallInst>(I).getCalledFunction()->getName() ==
				     "__VERIFIER_assume_internal"))
				points.push_back(&I);
		for (auto *I : points) {
			auto it = I->getIterator();
			auto *ci = llvm::dyn_cast<llvm::CallInst>(I);
			auto *callee = ci ? ci->getCalledFunction() : nullptr;
			auto *kind =
				callee && callee->getName() == "__VERIFIER_assume_internal"
					? llvm::dyn_cast<llvm::ConstantInt>(ci->getArgOperand(1))
					: nullptr;
			for (auto &[flag, type] : fs) {
				auto *set = new llvm::LoadInst(i1, flag, "", it);
				if (kind &&
				    kind->getZExtValue() == static_cast<std::uint64_t>(type)) {
					/* a skipped read fails the assume ... */
					auto *c = ci->getArgOperand(0);
					auto *bad = llvm::BinaryOperator::CreateAnd(set, c, "", it);
					llvm::CallInst::Create(
						assume,
						{llvm::BinaryOperator::CreateNot(bad, "", it)}, "",
						it);
					/* ... which consumes the flag */
					new llvm::StoreInst(llvm::ConstantInt::getFalse(ctx), flag,
							    std::next(it));
				} else if (!kind) {
					llvm::CallInst::Create(
						assume,
						{llvm::BinaryOperator::CreateNot(set, "", it)}, "",
						it);
				}
			}
		}
	}
	makeCheckable(&*clone, kinds);
	return printModule(*clone);
}

void writeFile(const std::string &filename, const std::string &content)
{
	std::error_code ec;
	llvm::raw_fd_ostream out(filename, ec);
	ERROR_ON(ec, "Could not write {}: {}\n", filename, ec.message());
	out << content;
}

} // namespace

auto PassChecker::kinds() const -> AnnotationKinds
{
	return {.helper = annotHelper, .confirmation = annotConf, .finalWrite = annotFinal};
}

void PassChecker::registerCallbacks(llvm::PassInstrumentationCallbacks &PIC)
{
	if (!outDir.empty()) {
		std::error_code ec = llvm::sys::fs::create_directories(outDir);
		ERROR_ON(ec, "Could not create directory {}: {}\n", outDir, ec.message());
		PIC.registerBeforeNonSkippedPassCallback(
			[this](llvm::StringRef passID, llvm::Any IR) { before(passID, IR); });
		PIC.registerAfterPassInvalidatedCallback(
			[this](llvm::StringRef passID, const llvm::PreservedAnalyses &) {
				if (!isPassContainer(passID))
					snapshots.pop_back();
			});
	}
	PIC.registerAfterPassCallback(
		[this](llvm::StringRef passID, llvm::Any IR, const llvm::PreservedAnalyses &PA) {
			after(passID, IR, PA);
		});
}

void PassChecker::writeAnnotationObligations(
	const llvm::Module &mod, const AnnotationInfo<llvm::Instruction *, llvm::Value *> &annots)
{
	if (outDir.empty() || annots.annotMap.empty())
		return;
	std::vector<std::string> unbuilt;
	auto tgt = printAnnotationObligationModule(mod, annots, kinds(), unbuilt);
	for (const auto &load : unbuilt)
		WARN("Load annotation uses a value not available at the load: {}\n", load);
	record("LoadAnnotations", "-", printCheckableModule(mod, kinds()), tgt);
}

void PassChecker::before(llvm::StringRef passID, llvm::Any IR)
{
	if (isPassContainer(passID))
		return;
	const auto *mod = getPassModule(IR);
	snapshots.push_back(mod ? printModule(*mod) : std::string());
}

void PassChecker::after(llvm::StringRef passID, llvm::Any IR, const llvm::PreservedAnalyses &PA)
{
	auto *fun = getPassFunction(IR);
	if (verifyAnalyses && fun) {
		/* The loop pass adaptor declares the tree preserved for its loop passes */
		if (llvm::any_cast<const llvm::Loop *>(&IR))
			loopPasses[fun].insert(passID.str());
		else
			verifyDominatorTree(passID, *fun, PA);
	}

	if (outDir.empty() || isPassContainer(passID))
		return;
	auto src = std::move(snapshots.back());
	snapshots.pop_back();
	const auto *mod = getPassModule(IR);
	if (!mod || printModule(*mod) == src)
		return;

	/* The module before the pass, back from its text */
	llvm::LLVMContext srcCtx;
	llvm::SMDiagnostic err;
	auto srcMod = llvm::parseAssemblyString(src, err, srcCtx);
	ERROR_ON(!srcMod, "Could not parse the module before {}\n", passID.str());

	/* Alive2 checks one function at a time: compare inlined calls inlined */
	auto inlineCalls = passID == "FunctionInlinerPass";
	record(passID.str(), fun ? fun->getName().str() : "-",
	       printCheckableModule(*srcMod, kinds(), inlineCalls),
	       passID == "SpinAssumePass" ? printSpinObligationModule(*mod, kinds())
					  : printCheckableModule(*mod, kinds(), inlineCalls));

	/* The dependencies of each event, in both directions: a target bit the
	 * source lacks is an added dependency, a missing one a dropped one */
	if (depMode != DepMode::None && checksDeps(passID)) {
		auto bits = assignSourceBits({&*srcMod, mod});
		auto scoped = depMode == DepMode::Scoped;
		auto name = fun ? fun->getName().str() : "-";
		auto tgt = printDepsModule(*mod, bits, scoped, kinds(), std::nullopt);
		std::string legend;
		for (const auto &[source, bit] : bits.bits)
			legend += std::to_string(bit) + " " + (source.empty() ? "?" : source) + "\n";
		for (auto check : {DepCheck::Added, DepCheck::Dropped}) {
			auto base = record(passID.str() + (check == DepCheck::Added ? "+deps-add"
										 : "+deps-drop"),
					   name, printDepsModule(*srcMod, bits, scoped, kinds(), check),
					   tgt);
			writeFile(base + ".bits.txt", legend);
		}
	}
}

void PassChecker::verifyDominatorTree(llvm::StringRef passID, llvm::Function &fun,
				      const llvm::PreservedAnalyses &PA)
{
	auto culprit = passID.str();
	if (auto it = loopPasses.find(&fun); it != loopPasses.end()) {
		culprit += " (loop passes:";
		for (const auto &name : it->second)
			culprit += " " + name;
		culprit += ")";
		loopPasses.erase(it);
	}
	if (!PA.getChecker<llvm::DominatorTreeAnalysis>().preserved())
		return;
	auto *DT = fam.getCachedResult<llvm::DominatorTreeAnalysis>(fun);
	if (DT && !DT->verify(llvm::DominatorTree::VerificationLevel::Full))
		ERROR("{} left a stale dominator tree in function {}\n", culprit,
		      fun.getName().str());
}

auto PassChecker::record(const std::string &pass, const std::string &fun, const std::string &src,
			 const std::string &tgt) -> std::string
{
	auto num = llvm::formatv("{0}", llvm::fmt_align(count++, llvm::AlignStyle::Right, 4, '0'))
			   .str();
	auto base = outDir + "/" + num + "-" + pass + "-" + (fun == "-" ? "module" : fun);
	writeFile(base + ".src.ll", src);
	writeFile(base + ".tgt.ll", tgt);

	std::error_code ec;
	llvm::raw_fd_ostream manifest(outDir + "/passes.txt", ec, llvm::sys::fs::OF_Append);
	ERROR_ON(ec, "Could not write {}/passes.txt: {}\n", outDir, ec.message());
	manifest << num << " " << pass << " " << fun << " " << base << ".src.ll " << base
		 << ".tgt.ll\n";
	return base;
}
