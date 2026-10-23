#include <gtest/gtest.h>

#include "StubChecker.hpp"
#include "genmc/Execution/DepExecutionGraph.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Verification/Rules/ErrorRules.hpp"

/*
 * The hb race rules of -exhaustive under IMM (#179), on hand-built graphs
 * whose hb views (view 0 of the stub checker) are given
 */

namespace {

struct RaceGraph {
	StubChecker checker;
	DepExecutionGraph g{ExecutionGraph::Config{.memModel = &checker}};
	SAddr addr = SAddr(0x100);

	RaceGraph()
	{
		auto tinfo = ThreadInfo{1, 0, 0, SVal(0), "t1"};
		auto *tcLab = g.add(ThreadCreateLabel::create(Event(0, 1), tinfo));
		tcLab->setViews({View()});
		g.addNewThread();
		auto *bLab = g.add(ThreadStartLabel::create(
			Event(1, 0), tcLab->getPos(), genmc::cast<ThreadCreateLabel>(tcLab), tinfo));
		bLab->setViews({View()});
	}

	/* Appends a write to thread T, with HB as its hb view */
	auto write(int t, MemOrdering ord, View hb = View()) -> const EventLabel *
	{
		auto *wLab = genmc::cast<WriteLabel>(g.add(WriteLabel::create(
			Event(t, g.getThreadSize(t)), ord, addr, ASize(4), SVal(1))));
		wLab->addCo(g.co_max(addr));
		hb.setMax(wLab->getPos());
		wLab->setViews({hb});
		return wLab;
	}

	/* Leaves a hole at the end of thread T, before what is added next */
	void hole(int t)
	{
		auto *fLab = g.add(FenceLabel::create(Event(t, g.getThreadSize(t)),
						      MemOrdering::SequentiallyConsistent));
		fLab->setViews({View()});
		g.relocateThread(t, fLab->getIndex(), 1);
		g.removeLast(t);
	}
};

auto upTo(Event e) -> View
{
	View v;
	v.setMax(e);
	return v;
}

} // namespace

TEST(RaceTest, UnorderedPlainWritesRace)
{
	RaceGraph r;
	const auto *w0 = r.write(0, MemOrdering::NotAtomic);
	const auto *w1 = r.write(1, MemOrdering::NotAtomic);

	auto race = error_rules::hbRaceWith(r.checker, r.g, w0, false);
	ASSERT_TRUE(race);
	EXPECT_EQ(race->type, VerificationError::VE_RaceNotAtomic);
	EXPECT_EQ(race->racyLab, w1);
	EXPECT_TRUE(error_rules::hbRace(r.checker, r.g));
}

TEST(RaceTest, AtomicWritesDoNotRace)
{
	RaceGraph r;
	const auto *w0 = r.write(0, MemOrdering::Relaxed);
	r.write(1, MemOrdering::Relaxed);

	EXPECT_FALSE(error_rules::hbRaceWith(r.checker, r.g, w0, false));
	EXPECT_FALSE(error_rules::hbRace(r.checker, r.g));
}

/* The access checked is hb-before the other one, as an event that fills a
 * hole can be before kept ones: ordered, whichever was added first */
TEST(RaceTest, AnAccessHbAfterTheCheckedOneDoesNotRace)
{
	RaceGraph r;
	const auto *w0 = r.write(0, MemOrdering::NotAtomic);
	const auto *w1 = r.write(1, MemOrdering::NotAtomic, upTo(w0->getPos()));

	EXPECT_FALSE(error_rules::hbRaceWith(r.checker, r.g, w0, false));
	EXPECT_FALSE(error_rules::hbRaceWith(r.checker, r.g, w1, false));
	EXPECT_FALSE(error_rules::hbRace(r.checker, r.g));
}

/* An access after a hole in its thread is not reached yet: not checked at
 * an event, but in the whole graph */
TEST(RaceTest, AccessesAfterAHoleAreSkippedOnRequest)
{
	RaceGraph r;
	const auto *w0 = r.write(0, MemOrdering::NotAtomic);
	r.hole(1);
	const auto *w1 = r.write(1, MemOrdering::NotAtomic);
	ASSERT_TRUE(genmc::isa<EmptyLabel>(r.g.getEventLabel(Event(1, 1))));

	EXPECT_FALSE(error_rules::hbRaceWith(r.checker, r.g, w0, true));
	auto race = error_rules::hbRaceWith(r.checker, r.g, w0, false);
	ASSERT_TRUE(race);
	EXPECT_EQ(race->racyLab, w1);
	EXPECT_TRUE(error_rules::hbRace(r.checker, r.g));
}

/* The whole-graph check reports a race at the access added later */
TEST(RaceTest, AGraphRaceIsReportedAtTheLaterAccess)
{
	RaceGraph r;
	const auto *w0 = r.write(0, MemOrdering::NotAtomic);
	const auto *w1 = r.write(1, MemOrdering::NotAtomic);
	ASSERT_LT(w0->getStamp(), w1->getStamp());

	auto race = error_rules::hbRace(r.checker, r.g);
	ASSERT_TRUE(race);
	EXPECT_EQ(race->pos, w1->getPos());
	EXPECT_EQ(race->racyLab, w0);
}
