#include <memory>

#include <gtest/gtest.h>

#include "StubChecker.hpp"
#include "genmc/ADT/DepView.hpp"
#include "genmc/ADT/View.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Verification/Checks/Invariants.hpp"

/* Moving a thread's events (ExecutionGraph::relocateThread()): positions,
 * the dependencies and views that name them, and the holes opened or closed */

namespace {

struct TestGraph {
	StubChecker checker;
	ExecutionGraph graph{ExecutionGraph::Config{.memModel = &checker}};

	/* Appends a fence to thread 0, with DEPS and a view seeing VIEWMAX of
	 * the thread */
	auto addFence(EventDeps deps = {}, int viewMax = 0) -> EventLabel *
	{
		auto pos = Event(0, graph.getThreadSize(0));
		auto lab = FenceLabel::create(pos, MemOrdering::SequentiallyConsistent);
		lab->setDeps(std::move(deps));
		View v;
		if (viewMax > 0)
			v.setMax(Event(0, viewMax));
		lab->setViews({v});
		return graph.add(std::move(lab));
	}
};

auto ctrlOn(std::initializer_list<int> idxs) -> EventDeps
{
	EventDeps deps;
	for (auto i : idxs)
		deps.ctrl.update(DepInfo(Event(0, i)));
	return deps;
}

} // namespace

TEST(RelocateTest, IndexMapping)
{
	/* Opening 2 slots at 3 */
	EXPECT_EQ(relocatedIndex(2, 3, 2), 2);
	EXPECT_EQ(relocatedIndex(3, 3, 2), 5);
	EXPECT_EQ(relocatedIndex(7, 3, 2), 9);
	/* Removing the slots [3, 5) */
	EXPECT_EQ(relocatedIndex(2, 5, -2), 2);
	EXPECT_EQ(relocatedIndex(3, 5, -2), 2);
	EXPECT_EQ(relocatedIndex(4, 5, -2), 2);
	EXPECT_EQ(relocatedIndex(5, 5, -2), 3);
}

TEST(RelocateTest, OpenThenCloseRestoresTheGraph)
{
	TestGraph g;
	g.addFence();			  /* (0, 1) */
	g.addFence(ctrlOn({1}), 1);	  /* (0, 2) */
	g.addFence(ctrlOn({2}), 2);	  /* (0, 3) */
	auto *last = g.addFence(ctrlOn({1, 3}), 3); /* (0, 4) */
	last->setPrefixView(std::make_unique<View>());

	g.graph.relocateThread(0, 2, 2);

	ASSERT_EQ(g.graph.getThreadSize(0), 7);
	for (auto i = 0; i < 7; i++)
		EXPECT_EQ(g.graph.getEventLabel(Event(0, i))->getPos(), Event(0, i));
	EXPECT_TRUE(genmc::isa<EmptyLabel>(g.graph.getEventLabel(Event(0, 2))));
	EXPECT_TRUE(genmc::isa<EmptyLabel>(g.graph.getEventLabel(Event(0, 3))));
	EXPECT_EQ(g.graph.getEventLabel(Event(0, 6)), last);
	EXPECT_TRUE(last->getDeps().ctrl.contains(Event(0, 1)));
	EXPECT_TRUE(last->getDeps().ctrl.contains(Event(0, 5)));
	EXPECT_FALSE(last->getDeps().ctrl.contains(Event(0, 3)));
	EXPECT_EQ(last->view(0).getMax(0), 5);
	EXPECT_FALSE(last->hasPrefixView()); /* dropped, to be recalculated */
	/* Below the move, nothing changes */
	EXPECT_EQ(g.graph.getEventLabel(Event(0, 4))->view(0).getMax(0), 1);
	EXPECT_TRUE(g.graph.getEventLabel(Event(0, 4))->getDeps().ctrl.contains(Event(0, 1)));
	/* po follows the positions, holes included */
	EXPECT_EQ(g.graph.po_imm_pred(last)->getPos(), Event(0, 5));
	EXPECT_EQ(g.graph.po_imm_succ(g.graph.getEventLabel(Event(0, 1)))->getPos(),
		  Event(0, 2));

	g.graph.relocateThread(0, 4, -2);

	ASSERT_EQ(g.graph.getThreadSize(0), 5);
	for (auto i = 0; i < 5; i++) {
		const auto *lab = g.graph.getEventLabel(Event(0, i));
		EXPECT_EQ(lab->getPos(), Event(0, i));
		EXPECT_FALSE(genmc::isa<EmptyLabel>(lab));
	}
	EXPECT_EQ(g.graph.getEventLabel(Event(0, 4)), last);
	EXPECT_TRUE(last->getDeps().ctrl.contains(Event(0, 3)));
	EXPECT_EQ(last->view(0).getMax(0), 3);
	EXPECT_EQ(g.graph.po_imm_pred(last)->getPos(), Event(0, 3));
}

TEST(RelocateTest, DepViewOpensHolesAndDropsRemovedOnes)
{
	/* Thread 0 seen up to 5, but for 3 */
	DepView v;
	v.setMax(Event(0, 5)); /* the indices it skips are holes */
	for (auto i : {1, 2, 4})
		v.removeHole(Event(0, i));
	v.setMax(Event(1, 2));
	v.removeAllHoles(1);
	ASSERT_FALSE(v.contains(Event(0, 3)));
	ASSERT_TRUE(v.contains(Event(0, 4)));

	/* The opened slots are not seen: they will hold events the view
	 * did not know of */
	v.relocateThread(0, 2, 2);
	EXPECT_EQ(v.getMax(0), 7);
	EXPECT_TRUE(v.contains(Event(0, 1)));
	EXPECT_FALSE(v.contains(Event(0, 2)));
	EXPECT_FALSE(v.contains(Event(0, 3)));
	EXPECT_TRUE(v.contains(Event(0, 4)));
	EXPECT_FALSE(v.contains(Event(0, 5)));
	EXPECT_TRUE(v.contains(Event(0, 7)));
	EXPECT_EQ(v.getMax(1), 2);

	v.relocateThread(0, 4, -2);
	EXPECT_EQ(v.getMax(0), 5);
	EXPECT_TRUE(v.contains(Event(0, 2)));
	EXPECT_FALSE(v.contains(Event(0, 3)));
	EXPECT_TRUE(v.contains(Event(0, 4)));
	EXPECT_TRUE(v.contains(Event(0, 5)));
}

TEST(RelocateTest, ViewBelowTheMoveIsUnchanged)
{
	View v;
	v.setMax(Event(0, 1));
	v.setMax(Event(2, 4));
	v.relocateThread(0, 2, 3);
	EXPECT_EQ(v.getMax(0), 1);
	v.relocateThread(2, 3, 1);
	EXPECT_EQ(v.getMax(2), 5);
}

/* A view that owns its storage and has a pending diff_ (it took the
 * diff_ while the storage was shared, then the other owner went away):
 * moving the thread down must not keep the diff_'s larger index */
TEST(RelocateTest, ViewWithPendingDiffMovesDown)
{
	View v;
	{
		View shared;
		shared.setMax(Event(1, 3));
		shared.setMax(Event(2, 4)); /* hydrates */
		v = shared;		    /* shares the storage */
		v.updateIdx(Event(0, 7));   /* a diff_, as the storage is shared */
	}
	ASSERT_EQ(v.getMax(0), 7);

	v.relocateThread(0, 5, -2);
	EXPECT_EQ(v.getMax(0), 5);
	EXPECT_EQ(v.getMax(1), 3);
	EXPECT_EQ(v.getMax(2), 4);
}

TEST(RelocateTest, ALabelFillsAHoleInTheLastSlot)
{
	/* A hole can be a thread's last label: the events a revisit kept after
	 * it are gone (#180). The label for its position fills it; it used to
	 * be appended after it, at a slot its position does not name, which
	 * DepExecutionGraph::cutToStamp() then freed but kept listed */
	TestGraph g;
	g.addFence(); /* (0, 1) */
	g.addFence(); /* (0, 2) */
	g.graph.relocateThread(0, 2, 1);
	g.graph.removeLast(0); /* the fence moved to (0, 3) */
	ASSERT_EQ(g.graph.getThreadSize(0), 3);
	ASSERT_TRUE(genmc::isa<EmptyLabel>(g.graph.getEventLabel(Event(0, 2))));

	auto lab = FenceLabel::create(Event(0, 2), MemOrdering::SequentiallyConsistent);
	lab->setViews({View()});
	auto *added = g.graph.add(std::move(lab));

	EXPECT_EQ(g.graph.getThreadSize(0), 3);
	EXPECT_EQ(g.graph.getEventLabel(Event(0, 2)), added);
	EXPECT_EQ(added->getPos(), Event(0, 2));
	EXPECT_EQ(g.graph.getLastThreadLabel(0), added);
	EXPECT_EQ(g.graph.po_imm_pred(added)->getPos(), Event(0, 1));
}

/* Closing slots removes holes, which may be the newest labels: the
 * graph's maximum stamp follows its newest label (#222, I1 failed on
 * data-structures/boost-queue under -lkmm -check-invariants) */
TEST(RelocateTest, ClosingSlotsKeepsTheMaximumStamp)
{
	TestGraph g;
	g.addFence(); /* (0, 1) */
	g.addFence(); /* (0, 2) */
	g.graph.relocateThread(0, 2, 2); /* the holes get the newest stamps */
	g.graph.relocateThread(0, 4, -2);

	EXPECT_EQ(g.graph.getMaxStamp(), g.graph.getLastThreadLabel(0)->getStamp());
	invariants::checkGraph(g.graph, {.where = "a unit test"});
}

/* A copy keeps a cached prefix view only if the copy has all of its
 * events: a label may be kept without its whole prefix (a write a
 * same-thread read reads from, see updatePredsWithPrefixView()), and its
 * view would name events the copy lacks (#222, I1 failed on
 * gobench/grpc3017 under -lkmm -check-invariants) */
TEST(RelocateTest, ACopyDropsPrefixViewsBeyondIt)
{
	TestGraph g;
	auto *f1 = g.addFence(); /* (0, 1) */
	auto *f2 = g.addFence(); /* (0, 2) */
	g.addFence();		 /* (0, 3) */
	View within;
	within.setMax(Event(0, 1));
	f1->setPrefixView(std::make_unique<View>(within));
	View beyond;
	beyond.setMax(Event(0, 3));
	f2->setPrefixView(std::make_unique<View>(beyond));

	View v;
	v.setMax(Event(0, 2));
	auto og = g.graph.getCopyUpTo(v);
	EXPECT_TRUE(og->getEventLabel(Event(0, 1))->hasPrefixView());
	EXPECT_FALSE(og->getEventLabel(Event(0, 2))->hasPrefixView());
	invariants::checkGraph(*og, {.where = "a unit test"});
}
