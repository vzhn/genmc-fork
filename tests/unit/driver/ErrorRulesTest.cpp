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
 * The memory-safety rules (Verification/Rules/ErrorRules.hpp) on a hand-built
 * state, without a driver.
 */

#include "genmc/Execution/ExecutionState.hpp"
#include "genmc/Verification/Rules/ErrorRules.hpp"

#include <gtest/gtest.h>

namespace {

struct Heap {
	ExecutionState st;
	SAddr block = st.onAlloc(Event(0, 1), ASize(16), 8, StorageDuration::SD_Heap,
				 StorageType::ST_Volatile, AddressSpace::AS_User);
	SAddr global = st.addStaticRange(ASize(8), 8, false, false, false);
};

} // namespace

TEST(ErrorRules, FreeingTheStartOfAHeapBlockIsValid)
{
	Heap h;
	EXPECT_FALSE(error_rules::invalidFreeReason(h.st, h.block, StorageDuration::SD_Heap));
}

TEST(ErrorRules, FreeingAGlobalIsInvalid)
{
	Heap h;
	auto why = error_rules::invalidFreeReason(h.st, h.global, StorageDuration::SD_Heap);
	ASSERT_TRUE(why);
	EXPECT_NE(why->find("global"), std::string::npos);
}

TEST(ErrorRules, FreeingInsideABlockIsInvalid)
{
	Heap h;
	auto why = error_rules::invalidFreeReason(h.st, h.block + 4, StorageDuration::SD_Heap);
	ASSERT_TRUE(why);
	EXPECT_NE(why->find("start"), std::string::npos);
}

TEST(ErrorRules, FreeingAnUnallocatedAddressIsInvalid)
{
	Heap h;
	/* (A freed block stays registered: double frees and accesses after
	 * free are races with the free, found on the graph) */
	auto other = SAddr::createHeap(0, 7, false, false);
	auto why = error_rules::invalidFreeReason(h.st, other, StorageDuration::SD_Heap);
	ASSERT_TRUE(why);
	EXPECT_NE(why->find("not in an allocated block"), std::string::npos);
}

TEST(ErrorRules, AccessesToLiveMemoryAreValid)
{
	Heap h;
	EXPECT_FALSE(error_rules::invalidAccess(h.st, true, Event(0, 3), AAccess(h.block, ASize(4))));
	EXPECT_FALSE(error_rules::invalidAccess(h.st, true, Event(0, 3), AAccess(h.global, ASize(4))));
}

TEST(ErrorRules, AccessToUnallocatedMemoryIsReportedAtTheAccess)
{
	Heap h;
	auto other = SAddr::createHeap(0, 7, false, false);
	auto err = error_rules::invalidAccess(h.st, true, Event(0, 3), AAccess(other, ASize(4)));
	ASSERT_TRUE(err);
	EXPECT_EQ(err->type, VerificationError::VE_AccessNonMalloc);
	EXPECT_EQ(err->pos, Event(0, 3));
}
