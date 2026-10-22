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
 * The resume-point store (Verification/ResumePointStore.hpp) with a fake
 * producer hook: which saved states are kept, and when they are dropped.
 */

#include "genmc/Verification/ResumePointStore.hpp"

#include <gtest/gtest.h>

namespace {

struct Fixture {
	ResumePointStore store{2};
	unsigned saves = 0;
	Fixture()
	{
		store.setHook([this](Event /*pos*/) {
			++saves;
			return std::make_shared<ResumePoint>();
		});
	}
};

} // namespace

TEST(ResumePointStore, WithoutAHookNothingIsSaved)
{
	ResumePointStore store{2};
	EXPECT_FALSE(store.hasHook());
	EXPECT_EQ(store.save(Event(0, 1)), nullptr);
	EXPECT_EQ(store.size(), 0U);
}

TEST(ResumePointStore, KeepsOnlyTheNewestStates)
{
	Fixture f;
	auto a = f.store.save(Event(0, 1));
	auto b = f.store.save(Event(0, 2));
	auto c = f.store.save(Event(0, 3));
	EXPECT_EQ(f.saves, 3U);
	EXPECT_EQ(f.store.size(), 2U);
	/* A dropped state stays valid for the revisits holding it */
	EXPECT_NE(a, nullptr);
}

TEST(ResumePointStore, AStateIsDroppedAfterItsLastRevisit)
{
	Fixture f;
	auto rp = f.store.save(Event(0, 1));
	rp->pending = 2; /* two forward revisits refer to it */

	f.store.resumeAt(rp);
	EXPECT_EQ(f.store.pending(), rp);
	EXPECT_EQ(f.store.size(), 1U);

	f.store.resumeAt(rp);
	EXPECT_EQ(f.store.size(), 0U);
	EXPECT_EQ(f.store.takePending(), rp);
	EXPECT_EQ(f.store.pending(), nullptr);
}

TEST(ResumePointStore, ARevisitWithoutAStateClearsThePendingOne)
{
	Fixture f;
	auto rp = f.store.save(Event(0, 1));
	rp->pending = 1;
	f.store.resumeAt(rp);
	f.store.resumeAt(nullptr);
	EXPECT_EQ(f.store.pending(), nullptr);
}
