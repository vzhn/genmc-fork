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
 * When a run repeats (Verification/State/KeptEventRuns.hpp), as a state machine
 * on its own.
 */

#include "genmc/Verification/State/KeptEventRuns.hpp"

#include <gtest/gtest.h>

using Repeat = KeptEventRuns::Repeat;

TEST(KeptEventRuns, ARunWithoutKeptEventsDoesNotRepeat)
{
	KeptEventRuns k;
	k.startRun(Stamp(5), 3, false);
	EXPECT_EQ(k.runStartStamp(), Stamp(5));
	EXPECT_EQ(k.runStartWork(), 3U);
	EXPECT_FALSE(k.takeDeferredCheck());
	EXPECT_EQ(k.endRun(false), Repeat::None);
}

TEST(KeptEventRuns, ARunRemembersWhetherItStartedWithKeptEvents)
{
	KeptEventRuns k;
	k.startRun(Stamp(5), 3, false);
	EXPECT_FALSE(k.startedWithKept());
	EXPECT_EQ(k.endRun(false), Repeat::None);
	k.startRun(Stamp(5), 3, true);
	EXPECT_TRUE(k.startedWithKept());
	EXPECT_EQ(k.endRun(false), Repeat::None);
}

TEST(KeptEventRuns, AnEventBeforeKeptOnesIsCheckedOnceAtTheEnd)
{
	KeptEventRuns k;
	k.startRun(Stamp(0), 0, true);
	k.addedBeforeKept();
	EXPECT_TRUE(k.takeAddedBeforeKept());
	EXPECT_FALSE(k.takeAddedBeforeKept());
	ASSERT_FALSE(k.eagerChecks());
	k.deferCheck();
	EXPECT_TRUE(k.takeDeferredCheck());
	EXPECT_FALSE(k.takeDeferredCheck());
	EXPECT_EQ(k.endRun(false), Repeat::None);
}

TEST(KeptEventRuns, AFailedDeferredCheckRepeatsTheRunEagerlyOnce)
{
	KeptEventRuns k;
	k.startRun(Stamp(0), 0, true);
	k.repeatEagerly();
	EXPECT_EQ(k.endRun(false), Repeat::Eagerly);
	EXPECT_TRUE(k.eagerChecks());

	/* The repeat is a run of its own; after it, checks are deferred again */
	k.startRun(Stamp(0), 0, true);
	EXPECT_EQ(k.endRun(false), Repeat::None);
	EXPECT_FALSE(k.eagerChecks());
}

TEST(KeptEventRuns, RelocationsAreCountedUntilARunSettles)
{
	KeptEventRuns k;
	for (auto i = 1U; i <= 3; i++) {
		k.startRun(Stamp(0), 0, true);
		k.repeatAfterRelocation();
		EXPECT_EQ(k.endRun(false), Repeat::AfterRelocation);
		EXPECT_EQ(k.relocationsInRun(), i);
	}
	k.startRun(Stamp(0), 0, true);
	EXPECT_EQ(k.endRun(false), Repeat::None);
	EXPECT_EQ(k.relocationsInRun(), 0U);
}

TEST(KeptEventRuns, ARelocationGoesBeforeAnEagerRepeat)
{
	KeptEventRuns k;
	k.startRun(Stamp(0), 0, true);
	k.repeatEagerly();
	k.repeatAfterRelocation();
	EXPECT_EQ(k.endRun(false), Repeat::AfterRelocation);
	EXPECT_EQ(k.endRun(false), Repeat::Eagerly);
}

TEST(KeptEventRuns, HaltingRepeatsNothing)
{
	KeptEventRuns k;
	k.startRun(Stamp(0), 0, true);
	k.repeatAfterRelocation();
	EXPECT_EQ(k.endRun(true), Repeat::None);
	EXPECT_EQ(k.relocationsInRun(), 0U);
}
