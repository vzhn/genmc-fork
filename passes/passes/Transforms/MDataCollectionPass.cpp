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

#include "MDataCollectionPass.hpp"
#include "genmc/Support/Error.hpp"
#include "genmc/Support/NameInfo.hpp"
#include "passes/LLVMUtils.hpp"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/Casting.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

using namespace llvm;

/* Strips typedefs and qualifiers off DIT */
static auto stripDIType(DIType *dit) -> DIType *
{
	while (auto *didt = dyn_cast_or_null<DIDerivedType>(dit)) {
		switch (didt->getTag()) {
		case dwarf::DW_TAG_typedef:
		case dwarf::DW_TAG_const_type:
		case dwarf::DW_TAG_volatile_type:
		case dwarf::DW_TAG_restrict_type:
		case dwarf::DW_TAG_atomic_type:
			if (!didt->getBaseType())
				return dit;
			dit = didt->getBaseType();
			break;
		default:
			return dit;
		}
	}
	return dit;
}

static void collectVarName(unsigned int ptr, DIType *dit, const std::string &nameBuilder,
			   NameInfo &info);

/* Names the elements of an array with dimensions COUNTS and elements of type ELEM */
static void collectArrayNames(unsigned int ptr, DIType *elem, uint64_t elemSize,
			      ArrayRef<uint64_t> counts, const std::string &nameBuilder,
			      NameInfo &info)
{
	if (counts.empty()) {
		collectVarName(ptr, elem, nameBuilder, info);
		return;
	}

	/* The stride of this dimension is the size of the subarray below it */
	auto stride = elemSize;
	for (auto count : counts.drop_front())
		stride *= count;
	for (auto i = 0U; i < counts.front(); i++)
		collectArrayNames(ptr + (i * stride), elem, elemSize, counts.drop_front(),
				  nameBuilder + "[" + std::to_string(i) + "]", info);
}

static void collectArrayNames(unsigned int ptr, DICompositeType *dict,
			      const std::string &nameBuilder, NameInfo &info)
{
	/* Arrays of unknown (count -1 or none) or zero size, like flexible array members,
	 * get a single name */
	SmallVector<uint64_t> counts;
	for (auto *node : dict->getElements()) {
		auto *subrange = dyn_cast<DISubrange>(node);
		auto *count = subrange ? dyn_cast_if_present<ConstantInt *>(subrange->getCount())
				       : nullptr;
		if (!count || count->getSExtValue() <= 0) {
			info.addOffsetInfo(ptr, nameBuilder);
			return;
		}
		counts.push_back(count->getSExtValue());
	}

	auto *base = stripDIType(dict->getBaseType());
	auto elemSize = base ? base->getSizeInBits() / 8 : 0;
	if (elemSize == 0) {
		info.addOffsetInfo(ptr, nameBuilder);
		return;
	}
	collectArrayNames(ptr, dict->getBaseType(), elemSize, counts, nameBuilder, info);
}

/* Names the data members of the struct, class, or union DICT */
static void collectMemberNames(unsigned int ptr, DICompositeType *dict,
			       const std::string &nameBuilder, NameInfo &info)
{
	auto isUnion = dict->getTag() == dwarf::DW_TAG_union_type;
	auto oldSize = info.size();
	std::optional<uint64_t> prevOffset;
	for (auto *elem : dict->getElements()) {
		/* Skip static members (DW_TAG_variable), methods, and virtual bases,
		 * which do not live at a fixed offset of the object */
		auto *member = dyn_cast<DIDerivedType>(elem);
		if (!member || member->isStaticMember() || member->isVirtual() ||
		    (member->getTag() != dwarf::DW_TAG_member &&
		     member->getTag() != dwarf::DW_TAG_inheritance))
			continue;

		/* Bitfields sharing a byte go by the first one's name */
		auto offset = member->getOffsetInBits() / 8;
		if (member->isBitField() && prevOffset == offset)
			continue;
		prevOffset = offset;

		/* Base classes and anonymous members add no component to the name */
		auto name = member->getName().empty() ? nameBuilder
						      : nameBuilder + "." + member->getName().str();
		collectVarName(ptr + offset, member->getBaseType(), name, info);

		/* All members of a union overlap: name it after the first one */
		if (isUnion)
			break;
	}

	/* Never leave an object without a name (e.g., an empty struct) */
	if (info.size() == oldSize)
		info.addOffsetInfo(ptr, nameBuilder);
}

/*
 * Names the object at offset PTR whose declared type is DIT.
 * We walk the debug-info type rather than the LLVM type of the allocation:
 * the latter is not the declared type when the object has a constant initializer
 * (e.g., the padding becomes explicit fields, an array becomes a packed struct,
 * a union takes the type of the initialized member).
 */
static void collectVarName(unsigned int ptr, DIType *dit, const std::string &nameBuilder,
			   NameInfo &info)
{
	auto *dict = dyn_cast_or_null<DICompositeType>(stripDIType(dit));
	if (!dict) {
		info.addOffsetInfo(ptr, nameBuilder);
		return;
	}

	switch (dict->getTag()) {
	case dwarf::DW_TAG_array_type:
		collectArrayNames(ptr, dict, nameBuilder, info);
		break;
	case dwarf::DW_TAG_structure_type:
	case dwarf::DW_TAG_class_type:
	case dwarf::DW_TAG_union_type:
		collectMemberNames(ptr, dict, nameBuilder, info);
		break;
	default:
		info.addOffsetInfo(ptr, nameBuilder);
		break;
	}
}

static auto collectVarName(DIType *dit) -> NameInfo
{
	NameInfo info;
	collectVarName(0, dit, "", info);
	return info;
}

void MDataInfo::collectGlobalInfo(GlobalVariable &v)
{
	/* If we already have data for the variable, skip */
	if (hasGlobalInfo(&v))
		return;

	auto &info = getGlobalInfo(&v);
	info = std::make_shared<NameInfo>();

	if (!v.getMetadata("dbg"))
		return;

	VERIFY(isa<DIGlobalVariableExpression>(v.getMetadata("dbg")));
	auto *dive = cast<DIGlobalVariableExpression>(v.getMetadata("dbg"));
	auto *dit = dive->getVariable()->getType();

	/* Check whether it is a global pointer */
	if (auto *ditc = dyn_cast_or_null<DIType>(dit))
		*info = collectVarName(ditc);
}

void MDataInfo::collectLocalInfo(DbgDeclareInst *dbgDecl)
{
	/* Skip if it's not an alloca or we don't have data */
	auto *v = dyn_cast<AllocaInst>(dbgDecl->getAddress());
	if (!v)
		return;

	if (hasLocalInfo(v))
		return;

	auto &info = getLocalInfo(v);
	info = std::make_shared<NameInfo>();

	/* Store alloca's metadata, in case it's used in memcpy */
	allocaMData[v] = dbgDecl->getVariable();

	auto *dit = dbgDecl->getVariable()->getType();
	if (auto *ditc = dyn_cast_or_null<DIType>(dit))
		*info = collectVarName(ditc);
}

void MDataInfo::collectMemCpyInfo(MemCpyInst *mi)
{
	/*
	 * Only mark global variables w/ private linkage that will
	 * otherwise go undetected by this pass
	 */
	auto *src = dyn_cast<GlobalVariable>(mi->getSource());
	if (!src)
		return;

	if (hasGlobalInfo(src))
		return;

	auto &info = getGlobalInfo(src);
	info = std::make_shared<NameInfo>();

	/*
	 * Since there will be no metadata for variables with private linkage,
	 * we do a small hack and take the metadata of memcpy()'s dest for
	 * memcpy()'s source
	 * The type of the dest is guaranteed to be a pointer
	 */
	auto *dst = dyn_cast<AllocaInst>(mi->getDest());
	VERIFY(dst);

	if (!allocaMData.contains(dst))
		return; /* We did our best, but couldn't get a name for it... */
	auto *dit = allocaMData[dst]->getType();
	if (auto *ditc = dyn_cast_or_null<DIType>(dit))
		*info = collectVarName(ditc);
}

auto MDataInfo::run(Module &M, ModuleAnalysisManager & /*AM*/) -> Result
{
	/* First, get type information for user's global variables */
	for (auto &v : GLOBALS(M))
		collectGlobalInfo(v);

	/* Then for all local variables and some other special cases */
	for (auto &F : M) {
		for (auto &I : instructions(F)) {
			if (auto *dbgDecl = dyn_cast<DbgDeclareInst>(&I))
				collectLocalInfo(dbgDecl);
			if (auto *mi = dyn_cast<MemCpyInst>(&I))
				collectMemCpyInfo(mi);
		}
	}
	return PMI;
}

auto MDataCollectionPass::run(Module &M, ModuleAnalysisManager &MAM) -> PreservedAnalyses
{
	PMI = MAM.getResult<MDataInfo>(M);
	return PreservedAnalyses::all();
}
