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
 * The revisits of the work list (Verification/State/Revisit.hpp,
 * WorkList.hpp): each kind follows a move of a thread's events (kept events
 * are relocated, see ExplorationCore::relocateExecution()), the read
 * revisits are found by their kind, and each prints as the debug output
 * shows it.
 */

#include "genmc/ADT/View.hpp"
#include "genmc/Verification/State/Revisit.hpp"
#include "genmc/Verification/State/WorkList.hpp"

#include <gtest/gtest.h>

#include <format>
#include <memory>

namespace {

/* Thread 1's events from index 2 on move up by 3 */
constexpr auto kThread = 1;
constexpr auto kFrom = 2;
constexpr auto kDelta = 3;

void relocate(Revisit &r) { r.relocateThread(kThread, kFrom, kDelta); }

/* (The formatter is the base's) */
auto str(const Revisit &r) -> std::string { return std::format("{}", r); }

auto view(Event e) -> std::unique_ptr<View>
{
	auto v = std::make_unique<View>();
	v->setMax(e);
	return v;
}

} // namespace

TEST(Revisit, EveryEventOfAReadRevisitMoves)
{
	ReadForwardRevisit r(Event(1, 2), Event(1, 4), true);
	relocate(r);
	EXPECT_EQ(r.getPos(), Event(1, 5));
	EXPECT_EQ(r.getRev(), Event(1, 7));
	EXPECT_TRUE(r.isMaximal());
}

TEST(Revisit, OnlyTheEventsFromTheMoveOnMove)
{
	ReadForwardRevisit r(Event(1, 1), Event(2, 4));
	relocate(r);
	EXPECT_EQ(r.getPos(), Event(1, 1));
	EXPECT_EQ(r.getRev(), Event(2, 4));
}

TEST(Revisit, ACoherenceRevisitMovesItsPredecessor)
{
	WriteForwardRevisit r(Event(0, 3), Event(1, 2));
	relocate(r);
	EXPECT_EQ(r.getPos(), Event(0, 3));
	EXPECT_EQ(r.getPred(), Event(1, 5));
}

TEST(Revisit, AWeakCasFailureMovesItsRevisitedRead)
{
	WeakCasFailureRevisit r(Event(1, 3), Event(1, 2));
	relocate(r);
	EXPECT_EQ(r.getPos(), Event(1, 6));
	EXPECT_EQ(r.getRev(), Event(1, 5));
}

TEST(Revisit, AReplayMovesTheErrorsPosition)
{
	ReplayForwardRevisit r(Event(1, 2), ErrorDetails(Event(1, 3), VerificationError::VE_Safety));
	relocate(r);
	EXPECT_EQ(r.getPos(), Event(1, 5));
	EXPECT_EQ(r.getDetails().pos, Event(1, 6));

	/* (An error without a position stays without) */
	ReplayForwardRevisit noPos(Event(1, 2), ErrorDetails(std::nullopt, VerificationError::VE_Liveness));
	relocate(noPos);
	EXPECT_FALSE(noPos.getDetails().pos);
}

TEST(Revisit, ABackwardRevisitMovesItsPrefix)
{
	BackwardRevisit r(Event(1, 2), Event(0, 4), view(Event(1, 3)));
	relocate(r);
	EXPECT_EQ(r.getPos(), Event(1, 5));
	EXPECT_EQ(r.getRev(), Event(0, 4));
	EXPECT_EQ(r.getViewNoRel()->getMax(1), 6);

	auto released = r.getViewRel();
	ASSERT_TRUE(released);
	EXPECT_FALSE(r.getViewNoRel());
	relocate(r); /* (without its prefix) */
	EXPECT_EQ(r.getPos(), Event(1, 8));
}

TEST(Revisit, AMoveDownMergesTheGapsEvents)
{
	/* Events in the closed gap go to the slot before it (see relocatedIndex()) */
	ReadForwardRevisit r(Event(1, 3), Event(1, 6));
	r.relocateThread(1, 5, -3);
	EXPECT_EQ(r.getPos(), Event(1, 1));
	EXPECT_EQ(r.getRev(), Event(1, 3));
}

TEST(Revisit, TheWorkListMovesEachItem)
{
	WorkList wl;
	wl.add(std::make_unique<OptionalForwardRevisit>(Event(1, 2)));
	wl.add(std::make_unique<RerunForwardRevisit>());
	wl.add(std::make_unique<WriteForwardRevisit>(Event(1, 4), Event(1, 3)));
	wl.relocateThread(kThread, kFrom, kDelta);

	auto mo = wl.getNext();
	ASSERT_TRUE(genmc::isa<WriteForwardRevisit>(&*mo));
	EXPECT_EQ(mo->getPos(), Event(1, 7));
	EXPECT_EQ(genmc::cast<WriteForwardRevisit>(&*mo)->getPred(), Event(1, 6));
	auto rerun = wl.getNext();
	EXPECT_EQ(rerun->getPos(), Event::getInit());
	EXPECT_EQ(wl.getNext()->getPos(), Event(1, 5));
	EXPECT_FALSE(wl.getNext());
}

TEST(Revisit, ReadRevisitsAreTheForwardAndBackwardOnes)
{
	ReadForwardRevisit fr(Event(1, 1), Event(2, 1));
	BackwardRevisit br(Event(1, 1), Event(2, 3), view(Event(1, 0)));
	WriteForwardRevisit mo(Event(1, 1), Event(2, 1));
	OptionalForwardRevisit opt(Event(1, 1));

	EXPECT_TRUE(genmc::isa<ReadRevisit>(static_cast<const Revisit *>(&fr)));
	EXPECT_TRUE(genmc::isa<ReadRevisit>(static_cast<const Revisit *>(&br)));
	EXPECT_FALSE(genmc::isa<ReadRevisit>(static_cast<const Revisit *>(&mo)));
	EXPECT_FALSE(genmc::isa<ReadRevisit>(static_cast<const Revisit *>(&opt)));

	const auto *rr = genmc::dyn_cast<ReadRevisit>(static_cast<const Revisit *>(&br));
	ASSERT_NE(rr, nullptr);
	EXPECT_EQ(rr->getRev(), Event(2, 3));
	EXPECT_EQ(rr->getRevisitKind(), Revisit::RV_BRev);
	EXPECT_EQ(genmc::dyn_cast<ReadRevisit>(static_cast<Revisit *>(&fr))->getRev(), Event(2, 1));
	EXPECT_TRUE(genmc::isa<ForwardRevisit>(static_cast<const Revisit *>(&opt)));
	EXPECT_FALSE(genmc::isa<ForwardRevisit>(static_cast<const Revisit *>(&br)));
}

TEST(Revisit, EachKindPrints)
{
	EXPECT_EQ(str(ReadForwardRevisit(Event(1, 1), Event(2, 3))), "FR((1, 1): (2, 3))");
	EXPECT_EQ(str(WriteForwardRevisit(Event(1, 1), Event(2, 3))), "MO((1, 1): (2, 3))");
	EXPECT_EQ(str(OptionalForwardRevisit(Event(1, 1))), "OPT((1, 1))");
	EXPECT_EQ(str(RerunForwardRevisit()), "RERUN");
	EXPECT_EQ(str(ReplayForwardRevisit(Event(1, 1), ErrorDetails())), "REPLAY");
	EXPECT_EQ(str(WeakCasFailureRevisit(Event(1, 1), Event(1, 0))), "WCF");
	auto br = BackwardRevisit(Event(1, 1), Event(2, 3), view(Event(1, 0)));
	EXPECT_EQ(str(br), std::format("BR((1, 1): [(2, 3), {}])", *br.getViewNoRel()));

	WorkList wl;
	wl.add(std::make_unique<OptionalForwardRevisit>(Event(1, 1)));
	wl.add(std::make_unique<RerunForwardRevisit>());
	EXPECT_EQ(std::format("{}", wl), "[ OPT((1, 1)) RERUN ]");
}
