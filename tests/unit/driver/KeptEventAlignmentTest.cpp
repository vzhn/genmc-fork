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
 * Kept-event alignment (Verification/Rules/KeptEventAlignment.hpp) on
 * hand-built threads, without a driver.
 */

#include "genmc/Execution/EventKey.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Execution/MemoryModel.hpp"
#include "genmc/Models/ConsistencyChecker.hpp"
#include "genmc/Support/ThreadInfo.hpp"
#include "genmc/Verification/Rules/KeptEventAlignment.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace {

using Kind = kept_events::Alignment::Kind;

/* What sits at a position of thread 1 */
struct Slot {
	enum : std::uint8_t { Hole, Own, Other, Unkeyed } what;
	std::uint32_t sub = 0; /* Own: the event's place in the instance */
};

constexpr auto kHole = Slot{.what = Slot::Hole};
constexpr auto kOther = Slot{.what = Slot::Other};
constexpr auto kUnkeyed = Slot{.what = Slot::Unkeyed};
constexpr auto own(std::uint32_t sub) -> Slot { return {.what = Slot::Own, .sub = sub}; }

/* Thread 1 holds SLOTS from index 1 on; the instance running is KEY */
struct KeptThread {
	std::unique_ptr<ConsistencyChecker> checker = ConsistencyChecker::create(ModelType::LKMM);
	ExecutionGraph g{ExecutionGraph::Config{.memModel = &*checker}};
	std::shared_ptr<const EventKey> key = std::make_shared<EventKey>(
		std::vector<EventKey::Step>{{7, 0}});
	std::shared_ptr<const EventKey> other = std::make_shared<EventKey>(
		std::vector<EventKey::Step>{{8, 0}});

	explicit KeptThread(const std::vector<Slot> &slots)
	{
		g.addNewThread();
		g.add(ThreadStartLabel::create(Event(1, 0), Event(0, 0), nullptr,
					       ThreadInfo{1, 0, 0, SVal(0), "t"}));
		for (const auto &slot : slots) {
			auto pos = Event(1, g.getThreadSize(1));
			if (slot.what == Slot::Hole) {
				g.add(EmptyLabel::create(pos));
				continue;
			}
			auto *lab = g.add(FenceLabel::create(pos, MemOrdering::Relaxed));
			if (slot.what == Slot::Own)
				lab->setKey(key, slot.sub);
			else if (slot.what == Slot::Other)
				lab->setKey(other, 0);
		}
	}

	/* The instance runs its SUB-th event at index IDX */
	auto align(int idx, std::uint32_t sub) const -> kept_events::Alignment
	{
		return kept_events::align(g, Event(1, idx), *key, sub);
	}
};

} // namespace

TEST(KeptEventAlignment, TheInstancesOwnEventIsReplayed)
{
	KeptThread t({own(1)});
	EXPECT_EQ(t.align(1, 1).kind, Kind::Own);
}

TEST(KeptEventAlignment, AnotherEventOfTheSameInstructionIsNotTheInstancesOwn)
{
	KeptThread t({own(2)});
	EXPECT_EQ(t.align(1, 1).kind, Kind::OpenGap);
}

TEST(KeptEventAlignment, AnEventWithoutAKeyIsAnError)
{
	KeptThread t({kUnkeyed, own(1)});
	EXPECT_EQ(t.align(1, 1).kind, Kind::MissingKey);
}

TEST(KeptEventAlignment, AnotherInstancesEventMakesRoom)
{
	KeptThread t({kOther, kHole, kOther});
	EXPECT_EQ(t.align(1, 1).kind, Kind::OpenGap);
}

TEST(KeptEventAlignment, AnotherInstancesEventBeforeTheOwnOneDropsTheRun)
{
	KeptThread t({kOther, kHole, own(1)});
	EXPECT_EQ(t.align(1, 1).kind, Kind::ShiftAndDrop);
}

TEST(KeptEventAlignment, AHoleAtTheEndIsFilled)
{
	KeptThread t({kOther, kHole, kHole});
	EXPECT_EQ(t.align(2, 1).kind, Kind::FillHole);
	EXPECT_EQ(t.align(4, 1).kind, Kind::FillHole); /* past the end */
}

TEST(KeptEventAlignment, TheOwnEventAfterHolesMovesHere)
{
	KeptThread t({kHole, kHole, own(1), kOther});
	EXPECT_EQ(t.align(1, 1), (kept_events::Alignment{.kind = Kind::MoveHere, .from = 3}));
}

TEST(KeptEventAlignment, OtherKeptEventsBeforeTheOwnOneDropTheRun)
{
	KeptThread t({kHole, kOther, own(1)});
	EXPECT_EQ(t.align(1, 1).kind, Kind::Drop);
}

TEST(KeptEventAlignment, AHoleBeforeOtherKeptEventsIsFilled)
{
	KeptThread t({kHole, kOther, own(2)});
	EXPECT_EQ(t.align(1, 1).kind, Kind::FillBeforeKept);
}
