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

#include "PromoteMemIntrinsicPass.hpp"
#include "genmc/Support/Error.hpp"

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/Alignment.h>
#include <llvm/Support/Casting.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <ranges>
#include <vector>

using namespace llvm;

/**
 * Lower a call to: __memcpy_chk(void * dest, const void * src, size_t len, size_t destlen);
 *
 * Reference implementation (see __memcpy_chk source):
 * 	if dstlen < len
 * 		__chk_fail ();
 * 	return memcpy(dest, src, len);
 *
 * __memcpy_chk spec:
 * http://refspecs.linux-foundation.org/LSB_4.0.0/LSB-Core-generic/LSB-Core-generic/libc---memcpy-chk-1.html
 * __memcpy_chk source:
 * https://sourceware.org/git/?p=glibc.git;a=blob;f=debug/memcpy_chk.c;h=6f8628ab945c5e8d2738f7382238c1e1006afe41;hb=HEAD
 * memcpy spec: https://pubs.opengroup.org/onlinepubs/9699919799/functions/memcpy.html
 */
static void lowerFortifiedMemCpy(CallInst *CI, Function &F)
{
	/* Get arguments */
	auto *dst_ptr = CI->getArgOperand(0);
	auto *src_ptr = CI->getArgOperand(1);
	auto *len = CI->getArgOperand(2);
	auto *dst_len = CI->getArgOperand(3);

	/* Split the basic block at the call to __memcpy_chk in bb (before call-instr.) and contBB
	 * (rest of the block) */
	auto *bb = CI->getParent();
	auto *contBB =
		bb->splitBasicBlock(CI); /* Creates a dummy terminator for bb -> remove later */

	/* Insert the llvm.memcpy intrinsic at the start of contBB */
	IRBuilder<> contBuilder(contBB, contBB->begin());
	contBuilder.CreateMemCpy(dst_ptr, Align(1), src_ptr, Align(1), len);
	CI->replaceAllUsesWith(
		dst_ptr); /* memcpy(...) returns the dst_ptr back, see: memcpy Spec */

	/* Create a new basic block "memcpy__chk_fail" for inside the IF => aborts using an
	 * unreachable-instruction */
	auto *failBB = BasicBlock::Create(F.getContext(), "memcpy__chk_fail", &F);
	auto *assertFailFun = F.getParent()->getFunction("__VERIFIER_assert_fail");
	CallInst::Create(assertFailFun, {}, "", failBB);
	new UnreachableInst(F.getContext(), failBB);

	/* Compare arguments: dstlen < len */
	auto *dummyTerminator = bb->getTerminator();
	IRBuilder<> termBuilder(dummyTerminator);
	auto *cmp = termBuilder.CreateICmpULT(dst_len, len);

	/* Replace the dummy terminator: We jump to failBB if the condition holds */
	dummyTerminator->eraseFromParent();
	termBuilder.SetInsertPoint(bb);
	termBuilder.CreateCondBr(cmp, /* True */ failBB, /* False */ contBB);
}

/**
 * We collect and lower all calls to the fortified version of memcpy: __memcpy_chk
 * The fortified version checks for buffer overflows before performing the memcpy.
 * @see lowerFortifiedMemCpy for implementation details
 */
static auto lowerFortifiedCalls(Function &F) -> bool
{
	auto modified = false;
	SmallVector<CallInst *, 8> fortifiedCalls;

	/* Collect call-instructions to __memcpy_chk */
	for (auto &I : instructions(F)) {
		auto *ci = dyn_cast<CallInst>(&I);
		if (!ci)
			continue;

		auto *calledFun = dyn_cast<Function>(ci->getCalledOperand());
		if (!calledFun || calledFun->getName() != "__memcpy_chk")
			continue;

		fortifiedCalls.push_back(ci);
		modified = true;
	}

	for (auto *ci : fortifiedCalls)
		lowerFortifiedMemCpy(ci, F);
	for (auto *ci : fortifiedCalls)
		ci->eraseFromParent();
	return modified;
}

/* A scalar field of an object: its byte offset and its type */
struct MemLeaf {
	uint64_t offset;
	Type *type;
};

/* The object a pointer points into: its type and the pointer's byte offset in it */
struct MemObject {
	Type *type;
	uint64_t offset;
};

/*
 * Find the type of the memory PTR points to.  With opaque pointers the type
 * comes from the instruction that produced the pointer: an alloca, a global
 * (a thread-local one through @llvm.threadlocal.address), or the source
 * element type of the GEP chain leading to it.  We walk the whole
 * constant-offset GEP chain, since LLVM canonicalizes field accesses to byte
 * offsets (getelementptr i8, ptr %p, i64 12), whose own type says nothing
 * about the memory.
 */
static auto getMemObject(const DataLayout &DL, Value *ptr) -> std::optional<MemObject>
{
	APInt offset(DL.getIndexTypeSizeInBits(ptr->getType()), 0);
	Type *typ = nullptr;
	auto fromGEP = false;

	for (auto *base = ptr;;) {
		if (auto *ai = dyn_cast<AllocaInst>(base)) {
			if (!ai->isArrayAllocation()) {
				typ = ai->getAllocatedType();
				fromGEP = false;
			}
			break;
		}
		if (auto *gv = dyn_cast<GlobalVariable>(base)) {
			typ = gv->getValueType();
			fromGEP = false;
			break;
		}
		if (auto *ii = dyn_cast<IntrinsicInst>(base);
		    ii && ii->getIntrinsicID() == Intrinsic::threadlocal_address) {
			base = ii->getArgOperand(0);
			continue;
		}
		auto *gep = dyn_cast<GEPOperator>(base);
		auto next = offset;
		if (!gep || !gep->accumulateConstantOffset(DL, next))
			break;
		offset = next;
		typ = gep->getSourceElementType();
		fromGEP = true;
		base = gep->getPointerOperand();
	}
	if (!typ || !typ->isSized() || offset.isNegative())
		return std::nullopt;

	auto size = DL.getTypeAllocSize(typ).getFixedValue();
	auto off = offset.getZExtValue();
	/* A GEP may step over whole elements first: p[3].f is still inside a typ */
	if (fromGEP && size && off >= size)
		off %= size;
	return MemObject{typ, off};
}

/* Append the scalar fields of TYP (placed at BASE) that overlap [lo, hi) */
static void collectLeaves(const DataLayout &DL, Type *typ, uint64_t base, uint64_t lo, uint64_t hi,
			  std::vector<MemLeaf> &leaves)
{
	auto size = DL.getTypeAllocSize(typ).getFixedValue();
	if (base >= hi || base + size <= lo)
		return;

	if (auto *st = dyn_cast<StructType>(typ)) {
		const auto *sl = DL.getStructLayout(st);
		for (auto i = 0U; i < st->getNumElements(); i++)
			collectLeaves(DL, st->getElementType(i), base + sl->getElementOffset(i), lo,
				      hi, leaves);
		return;
	}
	if (auto *at = dyn_cast<ArrayType>(typ)) {
		auto *elemTy = at->getElementType();
		auto stride = DL.getTypeAllocSize(elemTy).getFixedValue();
		if (stride == 0)
			return;
		/* Only visit the elements that overlap the range */
		auto first = lo > base ? (lo - base) / stride : 0;
		auto last = std::min<uint64_t>(at->getNumElements(), (hi - base + stride - 1) / stride);
		for (auto i = first; i < last; i++)
			collectLeaves(DL, elemTy, base + i * stride, lo, hi, leaves);
		return;
	}
	if (DL.getTypeStoreSize(typ).getFixedValue() == 0)
		return; /* e.g., empty struct members */
	leaves.push_back({base, typ});
}

/*
 * The scalar fields covering the LEN bytes at PTR, with offsets relative to
 * PTR.  Bytes not covered by any field are padding.  Fails if PTR's type is
 * unknown, if the range leaves the object, or if a field straddles one of its
 * ends (copying part of a field would create mixed-size accesses).
 */
static auto getLeaves(const DataLayout &DL, Value *ptr, uint64_t len)
	-> std::optional<std::vector<MemLeaf>>
{
	auto obj = getMemObject(DL, ptr);
	if (!obj || obj->offset + len > DL.getTypeAllocSize(obj->type).getFixedValue())
		return std::nullopt;

	auto lo = obj->offset;
	auto hi = lo + len;
	std::vector<MemLeaf> leaves;
	collectLeaves(DL, obj->type, 0, lo, hi, leaves);
	for (auto &leaf : leaves) {
		if (leaf.offset < lo ||
		    leaf.offset + DL.getTypeStoreSize(leaf.type).getFixedValue() > hi)
			return std::nullopt;
		leaf.offset -= lo;
	}
	return leaves;
}

static auto getLength(MemIntrinsic *MI) -> std::optional<uint64_t>
{
	auto *length = dyn_cast<ConstantInt>(MI->getLength());
	if (!length) {
		WARN_ONCE("memintr-length",
			  "Cannot promote non-constant-length mem intrinsic! Skipping...\n");
		return std::nullopt;
	}
	return length->getZExtValue();
}

static auto byteGEP(IRBuilder<> &builder, Value *ptr, uint64_t offset, const Twine &name) -> Value *
{
	return builder.CreateConstInBoundsGEP1_64(builder.getInt8Ty(), ptr, offset, name);
}

/*
 * Tries to promote a memcpy() to a load/store per scalar field.
 * The fields come from whichever side has a known type (dst first).  Both
 * sides are addressed by byte offset, so the other side may have a different
 * but compatible type, e.g.:
 *  - compatible: struct {i32, i32, i32} and [3 x i32]
 *  - compatible: %struct.S = {i32, i64} and the initializer type
 *    {i32, [4 x i8], i64} clang gives a constant of S
 *  - non-compatible: struct {i32, i64, i32} and [4 x i32]
 * Promotion in the last case would lead to "mixed-sized accesses".
 * Pointer operands may also be constant expressions, e.g.:
 *
 * call void @llvm.memcpy.p0.p0.i64(
 *      ptr align 8 getelementptr inbounds (%struct.queue_t, ptr @queue, i64 0, i32 0, i32 1),
 *      ptr align 8 %_3,
 *      i64 8,
 *      i1 false)
 */
static auto tryPromoteMemCpy(MemCpyInst *MI, SmallVector<llvm::MemIntrinsic *, 8> &promoted) -> bool
{
	auto len = getLength(MI);
	if (!len)
		return false;

	/* Remove memcpy's with len=0 completely as no-ops (generated by rustc) */
	if (*len == 0) {
		WARN_ONCE("memintr-zero-length",
			  "Cannot promote zero-length mem intrinsic! Removing instruction...\n");
		promoted.push_back(MI);
		return true;
	}

	/* Make sure we are not copying from genmc's space to userspace */
	VERIFY(MI->getSourceAddressSpace() == MI->getDestAddressSpace());

	const auto &DL = MI->getModule()->getDataLayout();
	auto leaves = getLeaves(DL, MI->getDest(), *len);
	if (!leaves)
		leaves = getLeaves(DL, MI->getSource(), *len);
	if (!leaves) {
		WARN_ONCE("memintr-opaque", "Cannot promote memcpy() due to both src and dst being "
					    "opaque or not matching the length! Skipping...\n");
		return false;
	}

	/*
	 * We produce a warning anyway because, e.g., if a small struct has no atomic
	 * fields, clang might initialize it with memcpy(), and then read it with a
	 * 64bit access, and mixed-size accesses are __bad__ news
	 */
	WARN_ONCE("promote-memintrinsic", "Memory intrinsic found! Attempting to promote it...\n");
	IRBuilder<> builder(MI);
	for (const auto &leaf : *leaves) {
		auto *src = byteGEP(builder, MI->getSource(), leaf.offset, "memcpy.src.gep");
		auto *dst = byteGEP(builder, MI->getDest(), leaf.offset, "memcpy.dst.gep");
		builder.CreateStore(builder.CreateLoad(leaf.type, src, "memcpy.src.load"), dst);
	}
	promoted.push_back(MI);
	return true;
}

/* The constant of type TYP whose every byte is BYTE */
static auto getSplatConstant(const DataLayout &DL, Type *typ, uint8_t byte) -> Constant *
{
	auto bits = DL.getTypeSizeInBits(typ).getFixedValue();
	auto val = bits < 8 ? APInt(bits, byte & ((1U << bits) - 1))
			    : APInt::getSplat(bits, APInt(8, byte));

	if (typ->isIntegerTy())
		return ConstantInt::get(typ, val);
	if (typ->isPointerTy())
		return byte == 0 ? Constant::getNullValue(typ)
				 : ConstantExpr::getIntToPtr(
					   ConstantInt::get(typ->getContext(), val), typ);
	return ConstantExpr::getBitCast(ConstantInt::get(typ->getContext(), val), typ);
}

/* Tries to promote a memset() to a store per scalar field */
static auto tryPromoteMemSet(MemSetInst *MS, SmallVector<MemIntrinsic *, 8> &promoted) -> bool
{
	auto len = getLength(MS);
	if (!len)
		return false;
	if (*len == 0) {
		WARN_ONCE("memintr-zero-length",
			  "Cannot promote zero-length mem intrinsic! Removing instruction...\n");
		promoted.push_back(MS);
		return true;
	}

	auto *val = dyn_cast<ConstantInt>(MS->getValue());
	if (!val) {
		WARN_ONCE("memintr-val",
			  "Cannot promote memset() with a non-constant value! Skipping...\n");
		return false;
	}

	const auto &DL = MS->getModule()->getDataLayout();
	auto leaves = getLeaves(DL, MS->getDest(), *len);
	if (!leaves) {
		WARN_ONCE("memintr-dst", "Cannot promote memset() due to dst being opaque or not "
					 "matching the length! Skipping...\n");
		return false;
	}

	WARN_ONCE("promote-memintrinsic", "Memory intrinsic found! Attempting to promote it...\n");
	IRBuilder<> builder(MS);
	auto byte = static_cast<uint8_t>(val->getZExtValue());
	for (const auto &leaf : *leaves)
		builder.CreateStore(getSplatConstant(DL, leaf.type, byte),
				    byteGEP(builder, MS->getDest(), leaf.offset, "memset.dst.gep"));
	promoted.push_back(MS);
	return true;
}

static void removePromoted(std::ranges::input_range auto &&promoted)
{
	for (auto *MI : promoted) {
		/* Are MI's operands used anywhere else? */
		auto *dst = dyn_cast<Instruction>(MI->getRawDest());
		Instruction *src = nullptr;
		if (auto *MC = dyn_cast<MemCpyInst>(MI))
			src = dyn_cast<Instruction>(MC->getRawSource());

		MI->eraseFromParent();
		if (dst && isa<GetElementPtrInst, CastInst>(dst) && dst->use_empty())
			dst->eraseFromParent();
		if (src && src != dst && isa<GetElementPtrInst, CastInst>(src) && src->use_empty())
			src->eraseFromParent();
	}
}

auto PromoteMemIntrinsicPass::run(Function &F, FunctionAnalysisManager & /*FAM*/)
	-> PreservedAnalyses
{
	/* Locate mem intrinsics of interest */
	SmallVector<llvm::MemIntrinsic *, 8> promoted;
	auto modified = false;

	modified |= lowerFortifiedCalls(F);
	for (auto &I : instructions(F)) {
		if (auto *MI = dyn_cast<MemCpyInst>(&I))
			modified |= tryPromoteMemCpy(MI, promoted);
		if (auto *MS = dyn_cast<MemSetInst>(&I))
			modified |= tryPromoteMemSet(MS, promoted);
	}

	/* Erase promoted intrinsics from the code */
	removePromoted(promoted);
	return modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
