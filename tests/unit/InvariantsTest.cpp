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
 * The invariant checks of -check-invariants (Verification/Checks/
 * Invariants.hpp) fire: each on a graph or state broken in the way it
 * describes, and none on the well-formed one.
 */

#include "StubChecker.hpp"
#include "genmc/ADT/DepView.hpp"
#include "genmc/Execution/DepExecutionGraph.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Execution/ExecutionState.hpp"
#include "genmc/Support/RMWOps.hpp"
#include "genmc/Support/ThreadInfo.hpp"
#include "genmc/Verification/Checks/Invariants.hpp"

#include <gtest/gtest.h>

namespace {

const invariants::Context kCtx{.where = "a unit test"};

/* Main creates thread 1 (at (0, 1)); both may access x and y */
struct Graph {
	StubChecker checker;
	ExecutionState st;
	ExecutionGraph g{ExecutionGraph::Config{
		.execState = &st, .memModel = &checker, .emitNALabels = false}};
	SAddr x = SAddr::createHeap(0, 1, false, false);
	SAddr y = SAddr::createHeap(0, 2, false, false);
	ThreadCreateLabel *tc{};
	ThreadStartLabel *begin{};

	explicit Graph(int childId = 1, int beginId = 1)
	{
		tc = add(ThreadCreateLabel::create(next(0), ThreadInfo{childId, 0, 0, SVal(0), "t"}));
		g.addNewThread();
		begin = add(ThreadStartLabel::create(Event(1, 0), tc->getPos(), tc,
						     ThreadInfo{beginId, 0, 0, SVal(0), "t"}));
	}

	auto next(int tid) const -> Event { return {tid, g.getThreadSize(tid)}; }

	template <typename LabelT> auto add(std::unique_ptr<LabelT> lab) -> LabelT *
	{
		lab->setViews({View()});
		return g.add(std::move(lab));
	}

	auto fence(int tid) -> FenceLabel *
	{
		return add(FenceLabel::create(next(tid), MemOrdering::SequentiallyConsistent));
	}

	auto write(int tid, SAddr addr) -> WriteLabel *
	{
		auto *wLab = add(
			WriteLabel::create(next(tid), MemOrdering::Relaxed, addr, ASize(4), SVal(1)));
		wLab->addCo(g.co_max(addr));
		return wLab;
	}

	auto read(int tid, SAddr addr, EventLabel *rf) -> ReadLabel *
	{
		auto *rLab = add(ReadLabel::create(next(tid), MemOrdering::Relaxed, addr, ASize(4),
						   nullptr, std::nullopt));
		rLab->setRf(rf);
		return rLab;
	}

	/* Thread 1 finishes; main joins it */
	auto finishAndJoin() -> std::pair<ThreadFinishLabel *, ThreadJoinLabel *>
	{
		auto *eLab = add(ThreadFinishLabel::create(next(1), SVal(0)));
		auto *jLab = add(ThreadJoinLabel::create(next(0), 1));
		eLab->setParentJoin(jLab);
		return {eLab, jLab};
	}
};

/* A graph that tracks dependencies, so that it may have holes: thread 0
 * gets fences at 1 and 2, then a hole at 2 (the fence moves to 3) */
struct HoleGraph {
	StubChecker checker;
	DepExecutionGraph g{ExecutionGraph::Config{.memModel = &checker}};

	HoleGraph()
	{
		fence(1);
		fence(2);
		g.relocateThread(0, 2, 1);
	}

	auto fence(int i) -> FenceLabel *
	{
		auto lab = FenceLabel::create(Event(0, i), MemOrdering::SequentiallyConsistent);
		lab->setViews({View()});
		return g.add(std::move(lab));
	}
};

} // namespace

/*******************************************************************************
 **                        I1: the graph
 ******************************************************************************/

TEST(InvariantsI1, AWellFormedGraphPasses)
{
	Graph t;
	auto *w = t.write(0, t.x);
	t.read(1, t.x, w);
	t.read(1, t.y, t.g.getInitLabel());
	t.finishAndJoin();
	invariants::checkGraph(t.g, kCtx);
}

TEST(InvariantsI1, AHoleAtTheEndOfAThread)
{
	HoleGraph h;
	h.g.removeLast(0);
	EXPECT_DEATH(invariants::checkGraph(h.g, kCtx), "thread 0 ends with a hole");
}

TEST(InvariantsI1, AHoleNeedsDependencyTracking)
{
	HoleGraph h;
	invariants::checkGraph(h.g, invariants::Context{.where = "a test", .holesAllowed = true});
	EXPECT_DEATH(invariants::checkGraph(h.g, kCtx), "without dependency tracking");
}

TEST(InvariantsI1, ABeginCarriesItsThreadsId)
{
	Graph t(1, 5);
	EXPECT_DEATH(invariants::checkGraph(t.g, kCtx), "the begin of thread 1 carries thread id 5");
}

TEST(InvariantsI1, ACreateNamesItsThreadAsTheChild)
{
	Graph t(2, 1);
	EXPECT_DEATH(invariants::checkGraph(t.g, kCtx), "names thread 2 as its child");
}

TEST(InvariantsI1, ABeginWithItsCreateIsNotTerminated)
{
	Graph t;
	t.begin->setTerminated();
	EXPECT_DEATH(invariants::checkGraph(t.g, kCtx), "is terminated although its create");
}

TEST(InvariantsI1, AThreadWhoseCreateWasCutHasOnlyItsBegin)
{
	Graph t;
	t.fence(1);
	t.g.removeLast(0); /* the create */
	EXPECT_DEATH(invariants::checkGraph(t.g, kCtx), "thread 1 has 2 labels although its create");
}

TEST(InvariantsI1, AReadReadsAWriteToItsAddress)
{
	Graph t;
	auto *w = t.write(0, t.x);
	t.read(1, t.y, w);
	EXPECT_DEATH(invariants::checkGraph(t.g, kCtx), "a write to another address");
}

TEST(InvariantsI1, AJoinedThreadEndsWithAFinish)
{
	Graph t;
	t.add(ThreadJoinLabel::create(t.next(0), 1));
	EXPECT_DEATH(invariants::checkGraph(t.g, kCtx), "whose last label is not a finish");
}

TEST(InvariantsI1, AFinishPointsAtItsJoin)
{
	Graph t;
	auto [eLab, jLab] = t.finishAndJoin();
	eLab->setParentJoin(nullptr);
	EXPECT_DEATH(invariants::checkGraph(t.g, kCtx), "is not linked to its join");
}

TEST(InvariantsI1, ViewsStayWithinTheThreads)
{
	Graph t;
	auto *f = t.fence(1);
	View v;
	v.setMax(Event(0, 7));
	f->setViews({v});
	EXPECT_DEATH(invariants::checkGraph(t.g, kCtx), "the view of \\(1, 1\\) reaches \\(0, 7\\)");
}

TEST(InvariantsI1, AViewToCopyStaysWithinTheThreads)
{
	Graph t;
	View v;
	v.setMax(Event(1, 0));
	invariants::checkCopyView(t.g, v, kCtx);
	v.setMax(Event(1, 3));
	EXPECT_DEATH(invariants::checkCopyView(t.g, v, kCtx), "the view to copy reaches \\(1, 3\\)");
}

TEST(InvariantsI1, ACutKeepsExactlyThePlannedLabels)
{
	Graph t;
	auto *f = t.fence(0);
	t.fence(1);
	auto plan = invariants::planCutToStamp(t.g, f->getStamp());
	t.g.cutToStamp(f->getStamp());
	invariants::checkGraph(t.g, kCtx, &plan);

	/* A label the plan does not keep, and one it keeps that is gone.
	 * (Fences: removeLast() leaves a write in coherence) */
	t.fence(1);
	EXPECT_DEATH(invariants::checkGraph(t.g, kCtx, &plan), "survived the cut to stamp");
	t.g.removeLast(1);
	t.g.removeLast(0);
	EXPECT_DEATH(invariants::checkGraph(t.g, kCtx, &plan), "was to be kept by the cut");
}

TEST(InvariantsI1, ACopyKeepsTheLabelsOfItsView)
{
	Graph t;
	t.write(0, t.x);
	t.write(1, t.y);
	View v;
	v.setMax(Event(0, 1));
	v.setMax(Event(1, 0));
	auto plan = invariants::planCopyUpTo(t.g, v);
	auto og = t.g.getCopyUpTo(v);
	invariants::checkGraph(*og, kCtx, &plan);
	/* (The copy against a plan of the whole graph) */
	View all;
	all.setMax(Event(0, 2));
	all.setMax(Event(1, 1));
	auto whole = invariants::planCopyUpTo(t.g, all);
	EXPECT_DEATH(invariants::checkGraph(*og, kCtx, &whole), "was to be kept by the copy");
}

/* The write part of an RMW comes right after its read (#223: a revisit
 * kept the write part without its read; the exploration crashed later, on
 * a revisit of the hole left) */
TEST(InvariantsI1, AnRMWWriteFollowsItsRead)
{
	Graph t;
	auto *rLab = t.add(FaiReadLabel::create(t.next(1), MemOrdering::Relaxed, t.x, ASize(4),
						RMWBinOp::Add, SVal(1)));
	rLab->setRf(t.g.getInitLabel());
	auto *wLab = t.add(FaiWriteLabel::create(t.next(1), MemOrdering::Relaxed, t.x, ASize(4),
						 SVal(1)));
	wLab->addCo(t.g.co_max(t.x));
	invariants::checkGraph(t.g, kCtx);
}

TEST(InvariantsI1, AnRMWWriteWithoutItsRead)
{
	Graph t;
	t.fence(1);
	auto *wLab = t.add(FaiWriteLabel::create(t.next(1), MemOrdering::Relaxed, t.x, ASize(4),
						 SVal(1)));
	wLab->addCo(t.g.co_max(t.x));
	EXPECT_DEATH(invariants::checkGraph(t.g, kCtx),
		     "the RMW write \\(1, 2\\) does not follow its read");
}

/* After the restriction for a revisit, the event it is for is there */
TEST(InvariantsI1, ARevisitIsForAnEventOfTheGraph)
{
	Graph t;
	auto *r = t.read(1, t.x, t.g.getInitLabel());
	auto *w = t.write(0, t.x);
	using invariants::RevisitTarget;
	invariants::checkRevisitTarget(t.g, r->getPos(), RevisitTarget::Read, w->getPos(), kCtx);
	invariants::checkRevisitTarget(t.g, w->getPos(), RevisitTarget::Write, std::nullopt, kCtx);
	EXPECT_DEATH(invariants::checkRevisitTarget(t.g, w->getPos(), RevisitTarget::Read,
						    std::nullopt, kCtx),
		     "the event \\(0, 2\\) a revisit is for is a W label");
	EXPECT_DEATH(invariants::checkRevisitTarget(t.g, Event(1, 5), RevisitTarget::Any,
						    std::nullopt, kCtx),
		     "the event \\(1, 5\\) a revisit is for is missing");
	EXPECT_DEATH(invariants::checkRevisitTarget(t.g, r->getPos(), RevisitTarget::Read,
						    Event(0, 1), kCtx),
		     "the write \\(0, 1\\) revisiting \\(1, 1\\) is a THREAD_CREATE label");
}

/*******************************************************************************
 **                        I3: the allocation map
 ******************************************************************************/

namespace {

/* Thread 1 allocates a block */
struct Heap : Graph {
	MallocLabel *alloc{};
	SAddr block;

	Heap()
	{
		auto pos = next(1);
		block = st.onAlloc(pos, ASize(8), 8, StorageDuration::SD_Heap,
				   StorageType::ST_Volatile, AddressSpace::AS_User);
		alloc = add(MallocLabel::create(pos, block, 8, 8, StorageDuration::SD_Heap,
						StorageType::ST_Volatile, AddressSpace::AS_User));
	}
};

} // namespace

TEST(InvariantsI3, AnAllocationAndItsFreeAreRegistered)
{
	Heap h;
	invariants::checkAllocations(h.g, h.st, kCtx);
	auto pos = h.next(1);
	h.add(FreeLabel::create(pos, h.block, ASize(8)));
	EXPECT_DEATH(invariants::checkAllocations(h.g, h.st, kCtx), "not registered as freed");
	h.st.onFree(pos, h.block, false);
	invariants::checkAllocations(h.g, h.st, kCtx);
}

TEST(InvariantsI3, AnAllocationIsRegistered)
{
	Heap h;
	auto other = SAddr::createHeap(1, 64, false, false);
	h.add(MallocLabel::create(h.next(1), other, 8, 8, StorageDuration::SD_Heap,
				  StorageType::ST_Volatile, AddressSpace::AS_User));
	EXPECT_DEATH(invariants::checkAllocations(h.g, h.st, kCtx), "is not registered");
}

TEST(InvariantsI3, TwoAllocationsHaveTwoAddresses)
{
	Heap h;
	h.add(MallocLabel::create(h.next(1), h.block, 8, 8, StorageDuration::SD_Heap,
				  StorageType::ST_Volatile, AddressSpace::AS_User));
	EXPECT_DEATH(invariants::checkAllocations(h.g, h.st, kCtx), "have the same address");
}

TEST(InvariantsI3, ARegisteredBlockBelongsToAnAllocationOfTheGraph)
{
	Heap h;
	h.st.onAlloc(Event(1, 7), ASize(4), 4, StorageDuration::SD_Heap, StorageType::ST_Volatile,
		     AddressSpace::AS_User);
	EXPECT_DEATH(invariants::checkAllocations(h.g, h.st, kCtx),
		     "which is not an allocation of the graph");
}

/*******************************************************************************
 **                        I4: resume points
 ******************************************************************************/

TEST(InvariantsI4, TheSameGraphMatchesItsFingerprint)
{
	Graph t;
	auto *w = t.write(0, t.x);
	t.read(1, t.x, w);
	auto fp = invariants::fingerprint(t.g, Event(1, 1));
	EXPECT_FALSE(invariants::resumeMismatch(t.g, fp, {Event(0, 2), Event(1, 1)}));
	invariants::checkResume(t.g, fp, {}, kCtx);
}

TEST(InvariantsI4, ALabelMoreIsAMismatch)
{
	Graph t;
	auto fp = invariants::fingerprint(t.g, Event::getInit());
	t.fence(1);
	auto mismatch = invariants::resumeMismatch(t.g, fp, {});
	ASSERT_TRUE(mismatch);
	EXPECT_NE(mismatch->find("thread 1 ends at index 1"), std::string::npos) << *mismatch;
	EXPECT_DEATH(invariants::checkResume(t.g, fp, {}, kCtx), "I4");
}

TEST(InvariantsI4, AThreadMoreIsAMismatch)
{
	Graph t;
	auto fp = invariants::fingerprint(t.g, Event::getInit());
	t.add(ThreadCreateLabel::create(t.next(0), ThreadInfo{2, 0, 0, SVal(0), "u"}));
	t.g.addNewThread();
	t.add(ThreadStartLabel::create(Event(2, 0), Event(0, 2), nullptr,
				       ThreadInfo{2, 0, 0, SVal(0), "u"}));
	auto mismatch = invariants::resumeMismatch(t.g, fp, {});
	ASSERT_TRUE(mismatch);
	EXPECT_NE(mismatch->find("has 3 threads"), std::string::npos) << *mismatch;
}

TEST(InvariantsI4, AnotherRfIsAMismatchExceptForTheSavedRead)
{
	Graph t;
	auto *w = t.write(0, t.x);
	auto *r = t.read(1, t.x, t.g.getInitLabel());
	auto fp = invariants::fingerprint(t.g, Event(0, 2));
	r->setRf(w);
	auto mismatch = invariants::resumeMismatch(t.g, fp, {});
	ASSERT_TRUE(mismatch);
	EXPECT_NE(mismatch->find("the labels of thread 1 differ"), std::string::npos) << *mismatch;

	/* The revisited read reads anew: its rf is not compared */
	auto saved = invariants::fingerprint(t.g, r->getPos());
	r->setRf(t.g.getInitLabel());
	EXPECT_FALSE(invariants::resumeMismatch(t.g, saved, {}));
}

TEST(InvariantsI4, AnotherCoherenceOrderIsAMismatch)
{
	Graph t;
	auto *w0 = t.write(0, t.x);
	auto *w1 = t.write(1, t.x);
	auto fp = invariants::fingerprint(t.g, Event::getInit());
	w1->moveCo(t.g.getInitLabel()); /* first in co */
	ASSERT_EQ(&*t.g.co(t.x).begin(), w1);
	auto mismatch = invariants::resumeMismatch(t.g, fp, {});
	ASSERT_TRUE(mismatch);
	EXPECT_NE(mismatch->find("the coherence order of"), std::string::npos) << *mismatch;
	(void)w0;
}

TEST(InvariantsI4, AStatesThreadIsNotAfterAHole)
{
	HoleGraph h;
	h.fence(4);
	auto fp = invariants::fingerprint(h.g, Event::getInit());
	EXPECT_FALSE(invariants::resumeMismatch(h.g, fp, {Event(0, 1)}));
	auto mismatch = invariants::resumeMismatch(h.g, fp, {Event(0, 3)});
	ASSERT_TRUE(mismatch);
	EXPECT_NE(mismatch->find("hole at (0, 2) before it"), std::string::npos) << *mismatch;
}

TEST(InvariantsI4, AResumedRunIsDrivenThroughItsHoles)
{
	HoleGraph h;
	auto ctx = invariants::Context{.where = "a test", .holesAllowed = true};
	invariants::checkResumeSchedule(h.g, {Event(0, 3)}, ctx);
	EXPECT_DEATH(invariants::checkResumeSchedule(h.g, {Event(0, 1)}, ctx),
		     "not driven through the hole at \\(0, 2\\)");
}

/*******************************************************************************
 **                        I5: consistency
 ******************************************************************************/

TEST(InvariantsI5, ACountedExecutionIsConsistent)
{
	Graph t;
	invariants::checkConsistency(t.g, true, kCtx);
	EXPECT_DEATH(invariants::checkConsistency(t.g, false, kCtx), "I5 \\(consistency\\)");
}
