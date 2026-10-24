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

#include "IntrinsicLoweringPass.hpp"
#include "genmc/Support/Error.hpp"

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Analysis/MemoryBuiltins.h>
#include <llvm/CodeGen/IntrinsicLowering.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/Casting.h>

#include <cstdint>
#include <iterator>
#include <memory>

using namespace llvm;

#define WITH_OVERFLOW_INST(I, EXT_OP, OP_BIN)                                                      \
	{                                                                                          \
		auto *OI = dyn_cast<WithOverflowInst>(I);                                          \
		IRBuilder<> builder(OI);                                                           \
                                                                                                   \
		auto *opA = OI->getArgOperand(0);                                                  \
		auto *opB = OI->getArgOperand(1);                                                  \
                                                                                                   \
		/* Get the type with twice the bitlength from our operands (i32 -> i64 etc.) */    \
		auto bitwidth = opA->getType()->getPrimitiveSizeInBits();                          \
		auto *typeExt = IntegerType::get(M.getContext(), bitwidth * 2);                    \
                                                                                                   \
		/* Extend the operands type to fit a possible overflow */                          \
		auto *aExt = builder.EXT_OP(opA, typeExt);                                         \
		auto *bExt = builder.EXT_OP(opB, typeExt);                                         \
                                                                                                   \
		/* Perform the operation to get the result */                                      \
		auto *result = builder.OP_BIN(opA, opB);                                           \
                                                                                                   \
		/* Perform same with extended bitwidth operands to check for overflows */          \
		auto *resultFromExtOps = builder.OP_BIN(aExt, bExt);                               \
		auto *resultExt = builder.EXT_OP(result, typeExt);                                 \
		auto *overflow = builder.CreateICmpNE(resultFromExtOps, resultExt);                \
                                                                                                   \
		/* Skip building the result-struct if it's only used by extractvals */             \
		if (replaceExtractValUsers(OI, result, overflow, toRemove)) {                      \
			/* Result struct is used somehow besides just extracting values */         \
			auto *resStruct = builder.CreateInsertValue(                               \
				UndefValue::get(OI->getType()), result, 0);                        \
			resStruct = builder.CreateInsertValue(resStruct, overflow, 1);             \
			OI->replaceAllUsesWith(resStruct);                                         \
		}                                                                                  \
		OI->eraseFromParent();                                                             \
                                                                                                   \
		modified = true;                                                                   \
		break;                                                                             \
	}

/* Given an overflowing instruction OI, replaces its extractval users with REPLACE or OVERFLOW.
 * Returns whether OI has any non-extractval users */
static auto replaceExtractValUsers(WithOverflowInst *oi, Value *replace, Value *overflow,
				   SmallVector<Instruction *, 8> &toRemove) -> bool
{
	auto result = false;
	for (auto *usr : oi->users()) {
		auto *evi = dyn_cast<ExtractValueInst>(usr);
		if (!evi) {
			result = true;
			continue;
		}

		auto indices = evi->getIndices();
		VERIFY(indices[0] == 0 || indices[0] == 1);
		if (indices[0] == 0)
			evi->replaceAllUsesWith(replace);
		if (indices[0] == 1)
			evi->replaceAllUsesWith(overflow);
		toRemove.push_back(evi);
	}
	return result;
}

/*
 * The floating-point intrinsics that IntrinsicLowering leaves to the code
 * generator (it reports a fatal error for them), as integer and select
 * instructions on scalar floats and doubles
 */

/* The bits of a float or double V, and its layout */
struct FPBits {
	Value *bits;
	unsigned width;
	unsigned mantissa; /* the mantissa's width */

	FPBits(IRBuilder<> &builder, Value *v)
		: width(v->getType()->getPrimitiveSizeInBits()),
		  mantissa(v->getType()->isFloatTy() ? 23 : 52)
	{
		bits = builder.CreateBitCast(v, builder.getIntNTy(width));
	}

	auto mask(IRBuilder<> &builder, uint64_t m) const -> Value *
	{
		return builder.getIntN(width, m);
	}
	auto signMask() const -> uint64_t { return 1ULL << (width - 1); }
	auto expMask() const -> uint64_t { return ~signMask() & ~mantMask(); }
	auto mantMask() const -> uint64_t { return (1ULL << mantissa) - 1; }
	auto quietMask() const -> uint64_t { return 1ULL << (mantissa - 1); }
};

static auto isLowerableFPType(const llvm::Type *type) -> bool
{
	return type->isFloatTy() || type->isDoubleTy();
}

/* fabs: clears the sign bit */
static auto lowerFAbs(IRBuilder<> &builder, Value *v) -> Value *
{
	FPBits fp(builder, v);
	auto *abs = builder.CreateAnd(fp.bits, fp.mask(builder, ~fp.signMask()));
	return builder.CreateBitCast(abs, v->getType());
}

/* is.fpclass: whether V is in one of the classes of MASK (see the LangRef) */
static auto lowerIsFPClass(IRBuilder<> &builder, Value *v, uint64_t classes) -> Value *
{
	FPBits fp(builder, v);
	auto *exp = builder.CreateAnd(fp.bits, fp.mask(builder, fp.expMask()));
	auto *mant = builder.CreateAnd(fp.bits, fp.mask(builder, fp.mantMask()));
	auto *quiet = builder.CreateAnd(fp.bits, fp.mask(builder, fp.quietMask()));
	auto *neg = builder.CreateICmpNE(
		builder.CreateAnd(fp.bits, fp.mask(builder, fp.signMask())), fp.mask(builder, 0));
	auto *pos = builder.CreateNot(neg);
	auto *expMax = builder.CreateICmpEQ(exp, fp.mask(builder, fp.expMask()));
	auto *expZero = builder.CreateICmpEQ(exp, fp.mask(builder, 0));
	auto *mantZero = builder.CreateICmpEQ(mant, fp.mask(builder, 0));
	auto *quietSet = builder.CreateICmpNE(quiet, fp.mask(builder, 0));

	auto *nan = builder.CreateAnd(expMax, builder.CreateNot(mantZero));
	auto *inf = builder.CreateAnd(expMax, mantZero);
	auto *zero = builder.CreateAnd(expZero, mantZero);
	auto *sub = builder.CreateAnd(expZero, builder.CreateNot(mantZero));
	auto *normal = builder.CreateNot(builder.CreateOr(expMax, expZero));
	Value *const cls[] = {
		builder.CreateAnd(nan, builder.CreateNot(quietSet)), /* signaling NaN */
		builder.CreateAnd(nan, quietSet),		     /* quiet NaN */
		builder.CreateAnd(inf, neg),
		builder.CreateAnd(normal, neg),
		builder.CreateAnd(sub, neg),
		builder.CreateAnd(zero, neg),
		builder.CreateAnd(zero, pos),
		builder.CreateAnd(sub, pos),
		builder.CreateAnd(normal, pos),
		builder.CreateAnd(inf, pos),
	};
	Value *result = builder.getFalse();
	for (auto i = 0U; i < std::size(cls); i++)
		if (classes & (1U << i))
			result = builder.CreateOr(result, cls[i]);
	return result;
}

/* minnum/maxnum: the smaller/larger operand; a NaN loses (if both are NaNs,
 * one is the result). Either zero for -0 and +0 */
static auto lowerMinMaxNum(IRBuilder<> &builder, Value *x, Value *y, bool isMin) -> Value *
{
	auto *xNaN = builder.CreateFCmpUNO(x, x);
	auto *yNaN = builder.CreateFCmpUNO(y, y);
	auto *pick = isMin ? builder.CreateFCmpOLT(x, y) : builder.CreateFCmpOGT(x, y);
	auto *ordered = builder.CreateSelect(pick, x, y);
	return builder.CreateSelect(xNaN, y, builder.CreateSelect(yNaN, x, ordered));
}

static auto runOnBasicBlock(BasicBlock &bb, IntrinsicLowering *IL) -> bool
{
	auto &M = *bb.getParent()->getParent();
	auto modified = false;
	SmallVector<Instruction *, 8> toRemove;

	for (auto it = bb.begin(); it != bb.end();) {
		auto *I = llvm::dyn_cast<IntrinsicInst>(&*it);
		/* Iterator is incremented in order for it not to be invalidated */
		++it;
		/* If the instruction is not an intrinsic call, skip it */
		if (!I)
			continue;
		switch (I->getIntrinsicID()) {
		/* Access thread-local variables directly: the interpreter resolves
		 * each use of one to the executing thread's copy */
		case llvm::Intrinsic::threadlocal_address:
			I->replaceAllUsesWith(I->getOperand(0));
			break;
		case llvm::Intrinsic::vastart:
		case llvm::Intrinsic::vaend:
		case llvm::Intrinsic::vacopy:
			break;
		case llvm::Intrinsic::dbg_value:
		case llvm::Intrinsic::dbg_declare:
			/* Remove useless calls to @llvm.debug.* */
			I->eraseFromParent();
			modified = true;
			break;
		case llvm::Intrinsic::trap: {
			/*
			 * Check for calls to @llvm.trap. Such calls may occur if LLVM
			 * detects a NULL pointer dereference in the CFG and simplify
			 * it to a trap call. In order for this to happen, the program
			 * has to be compiled with -O1 or -O2.
			 * We lower such calls to abort() (@trap could also be erased from M)
			 */
			auto FC = M.getOrInsertFunction("abort",
							llvm::Type::getVoidTy(M.getContext()));
			if (auto *F = llvm::dyn_cast<llvm::Function>(FC.getCallee())) {
				F->setDoesNotReturn();
				F->setDoesNotThrow();
				llvm::CallInst::Create(F, llvm::Twine(), I->getIterator());
			}
			I->eraseFromParent();
			modified = true;
			break;
		}
		case llvm::Intrinsic::objectsize: {
			auto *objSize = llvm::lowerObjectSizeCall(I,
								  I->getModule()->getDataLayout(),
								  nullptr, /* must succeed */ true);
			I->replaceAllUsesWith(objSize);
			I->eraseFromParent();
			modified = true;
			break;
		}
		case llvm::Intrinsic::fmuladd: {
			/* clang contracts x * y + z into it (-ffp-contract=on). It may
			 * be fused or not
			 * (https://llvm.org/docs/LangRef.html#llvm-fmuladd-intrinsic): a
			 * multiplication and an addition */
			IRBuilder<> builder(I);
			builder.setFastMathFlags(I->getFastMathFlags());
			auto *mul = builder.CreateFMul(I->getArgOperand(0), I->getArgOperand(1));
			I->replaceAllUsesWith(builder.CreateFAdd(mul, I->getArgOperand(2)));
			I->eraseFromParent();
			modified = true;
			break;
		}
		/* fabs(), isnan() and the like, fmin(), fmax() */
		case llvm::Intrinsic::fabs:
		case llvm::Intrinsic::is_fpclass:
		case llvm::Intrinsic::minnum:
		case llvm::Intrinsic::maxnum: {
			if (!isLowerableFPType(I->getArgOperand(0)->getType())) {
				IL->LowerIntrinsicCall(I);
				modified = true;
				break;
			}
			IRBuilder<> builder(I);
			auto *x = I->getArgOperand(0);
			Value *result = nullptr;
			switch (I->getIntrinsicID()) {
			case llvm::Intrinsic::fabs:
				result = lowerFAbs(builder, x);
				break;
			case llvm::Intrinsic::is_fpclass:
				result = lowerIsFPClass(
					builder, x,
					cast<ConstantInt>(I->getArgOperand(1))->getZExtValue());
				break;
			default:
				result = lowerMinMaxNum(builder, x, I->getArgOperand(1),
							I->getIntrinsicID() ==
								llvm::Intrinsic::minnum);
				break;
			}
			I->replaceAllUsesWith(result);
			I->eraseFromParent();
			modified = true;
			break;
		}
		// https://llvm.org/docs/LangRef.html#llvm-sadd-with-overflow-intrinsics
		case llvm::Intrinsic::sadd_with_overflow:
			WITH_OVERFLOW_INST(I, CreateSExt, CreateAdd)
		case llvm::Intrinsic::ssub_with_overflow:
			WITH_OVERFLOW_INST(I, CreateSExt, CreateSub)
		case llvm::Intrinsic::smul_with_overflow:
			WITH_OVERFLOW_INST(I, CreateSExt, CreateMul)
		case llvm::Intrinsic::uadd_with_overflow:
			WITH_OVERFLOW_INST(I, CreateZExt, CreateAdd)
		case llvm::Intrinsic::usub_with_overflow:
			WITH_OVERFLOW_INST(I, CreateZExt, CreateSub)
		case llvm::Intrinsic::umul_with_overflow:
			WITH_OVERFLOW_INST(I, CreateZExt, CreateMul)
		default:
			IL->LowerIntrinsicCall(I);
			modified = true;
			break;
		}
	}

	/* As removing other instructions while lowering *.with.overflows is unsafe while
	 * iterating over those same instructions, we do deletions separately */
	for (auto *inst : toRemove)
		inst->eraseFromParent();
	return modified;
}

auto IntrinsicLoweringPass::run(Function &F, FunctionAnalysisManager & /*FAM*/) -> PreservedAnalyses
{
	auto ILUP = std::make_unique<IntrinsicLowering>(F.getParent()->getDataLayout());

	/* Scan through the instructions and lower intrinsic calls */
	auto modified = false;
	for (auto &bb : F)
		modified |= runOnBasicBlock(bb, &*ILUP);

	return modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
