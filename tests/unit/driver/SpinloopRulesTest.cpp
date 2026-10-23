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
 * Spinloop detection without a driver: the side-effect tracking of loop
 * iterations (ExecutionState) on a hand-fed state, and the spinloop rules
 * (Verification/Rules/SpinloopRules.hpp) on hand-built RC11 graphs.
 */

#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Execution/ExecutionState.hpp"
#include "genmc/Execution/MemoryModel.hpp"
#include "genmc/Models/ConsistencyChecker.hpp"
#include "genmc/Support/ThreadInfo.hpp"
#include "genmc/Verification/Rules/SpinloopRules.hpp"

#include <gtest/gtest.h>

namespace {

constexpr auto kLoop = LoopKey{.id = 1};
constexpr auto kInner = LoopKey{.id = 2};

/* Thread 1 runs loops over a global and a block of its own */
struct Loops {
	using LockRole = ExecutionState::LockRole;

	ExecutionState st;
	SAddr global = st.addStaticRange(ASize(8), 8, false, false, false);
	SAddr own = st.onAlloc(Event(1, 1), ASize(8), 8, StorageDuration::SD_Heap,
			       StorageType::ST_Volatile, AddressSpace::AS_User);
	int next = 2; /* the next index in thread 1 */

	auto pos() -> Event { return {1, next++}; }
	void begin(LoopKey loop = kLoop) { st.onLoopBegin(pos(), loop); }
	/* Starts the next iteration; whether the last one had side effects */
	auto spin(LoopKey loop = kLoop) -> bool { return st.onSpinStart(pos(), loop); }
	void load(SAddr addr) { st.onATLoad(pos(), AAccess(addr, ASize(4)), View()); }
	void store(SAddr addr, bool isRelease = false, LockRole role = LockRole::None)
	{
		st.onATStore(pos(), AAccess(addr, ASize(4)), View(), true, isRelease, role);
	}
};

} // namespace

TEST(SpinloopDetection, AnIterationWithoutWritesHasNoSideEffects)
{
	Loops l;
	l.begin();
	l.load(l.global);
	EXPECT_FALSE(l.spin());
}

TEST(SpinloopDetection, AGlobalStoreIsASideEffectOfItsIterationOnly)
{
	Loops l;
	l.begin();
	l.store(l.global);
	EXPECT_TRUE(l.spin());
	EXPECT_FALSE(l.spin());
}

TEST(SpinloopDetection, AStoreToAnUnreadOwnBlockIsNoSideEffect)
{
	Loops l;
	l.begin();
	l.store(l.own);
	EXPECT_FALSE(l.spin());
}

TEST(SpinloopDetection, AStoreToAnOwnBlockTheIterationReadIsASideEffect)
{
	Loops l;
	l.begin();
	l.load(l.own);
	l.store(l.own);
	EXPECT_TRUE(l.spin());
}

TEST(SpinloopDetection, AReleaseStoreIsASideEffect)
{
	Loops l;
	l.begin();
	l.store(l.own, true);
	EXPECT_TRUE(l.spin());
}

TEST(SpinloopDetection, AMutexTakenAndReleasedIsNoSideEffect)
{
	Loops l;
	l.begin();
	l.store(l.global, false, Loops::LockRole::Acquire);
	l.store(l.global, true, Loops::LockRole::Release);
	EXPECT_FALSE(l.spin());
}

TEST(SpinloopDetection, AMutexStillTakenIsASideEffect)
{
	Loops l;
	l.begin();
	l.store(l.global, false, Loops::LockRole::Acquire);
	EXPECT_TRUE(l.spin());
}

TEST(SpinloopDetection, ReleasingAMutexTheIterationDidNotTakeIsASideEffect)
{
	Loops l;
	l.begin();
	l.store(l.global, true, Loops::LockRole::Release);
	EXPECT_TRUE(l.spin());
}

/* A wait lets the next iteration see what the signaller did */
TEST(SpinloopDetection, AMutexIterationThatWaitedIsASideEffect)
{
	Loops l;
	l.begin();
	l.store(l.global, false, Loops::LockRole::Acquire);
	l.st.onAwaitStart(l.pos());
	l.store(l.global, true, Loops::LockRole::Release);
	EXPECT_TRUE(l.spin());
}

TEST(SpinloopDetection, CreatingAThreadIsASideEffect)
{
	Loops l;
	l.begin();
	l.st.onThreadCreate(l.pos());
	EXPECT_TRUE(l.spin());
}

TEST(SpinloopDetection, AnInnerLoopsStoresCountForTheOuterIteration)
{
	Loops l;
	l.begin(kLoop);
	l.begin(kInner);
	EXPECT_FALSE(l.spin(kInner));
	l.store(l.global);
	EXPECT_TRUE(l.spin(kLoop));
}

namespace {

/*
 * Thread 1 runs an FAI-ZNE loop on x:
 *
 *   [W x]  (with PRESTORE: a store hb-before the loop)
 *   LoopBegin  SpinStart  FAI(x)  ZNE-end
 */
struct FaiLoop {
	std::unique_ptr<ConsistencyChecker> checker = ConsistencyChecker::create(ModelType::RC11);
	ExecutionGraph g{ExecutionGraph::Config{.memModel = &*checker, .emitNALabels = true}};
	SAddr x = SAddr::createStatic(0, 0, false, false);
	SAddr y = SAddr::createStatic(0, 8, false, false);

	explicit FaiLoop(bool preStore = false)
	{
		spawn(1);
		if (preStore)
			write(1, x);
		add(LoopBeginLabel::create(next(1), kLoop));
		spinStart = add(SpinStartLabel::create(next(1), kLoop));
		fai(1, x);
		zne = add(FaiZNESpinEndLabel::create(next(1)));
	}

	void spawn(int tid)
	{
		g.addNewThread();
		add(ThreadStartLabel::create(Event(tid, 0), Event(0, 0), nullptr,
					     ThreadInfo{tid, 0, 0, SVal(0), "t"}));
	}

	auto next(int tid) const -> Event { return {tid, g.getThreadSize(tid)}; }

	template <typename LabelT> auto add(std::unique_ptr<LabelT> lab) -> LabelT *
	{
		auto *added = g.add(std::move(lab));
		checker->updateMMViews(added);
		return added;
	}

	auto write(int tid, SAddr addr) -> WriteLabel *
	{
		auto *wLab = g.add(WriteLabel::create(next(tid), MemOrdering::Relaxed, addr,
						      ASize(4), SVal(1)));
		wLab->addCo(g.co_max(addr));
		checker->updateMMViews(wLab);
		return wLab;
	}

	auto fai(int tid, SAddr addr) -> FaiWriteLabel *
	{
		auto *rLab = g.add(FaiReadLabel::create(next(tid), MemOrdering::Relaxed, addr,
							ASize(4), RMWBinOp::Add, SVal(1)));
		rLab->setRf(g.co_max(addr));
		checker->updateMMViews(rLab);
		auto *wLab = g.add(FaiWriteLabel::create(next(tid), MemOrdering::Relaxed, addr,
							 ASize(4), SVal(1)));
		wLab->addCo(g.co_max(addr));
		checker->updateMMViews(wLab);
		return wLab;
	}

	/* Blocks thread 1 at its ZNE end */
	auto block() -> FaiZNEBlockLabel * { return add(FaiZNEBlockLabel::create(next(1))); }

	SpinStartLabel *spinStart{};
	FaiZNESpinEndLabel *zne{};
};

} // namespace

TEST(SpinloopRules, TheSpinStartAfterTheLoopBeginStartsTheFirstIteration)
{
	FaiLoop l;
	EXPECT_TRUE(spin_rules::startsFirstIteration(l.g, *l.spinStart));

	auto *again = l.add(SpinStartLabel::create(l.next(1), kLoop));
	EXPECT_FALSE(spin_rules::startsFirstIteration(l.g, *again));
}

TEST(SpinloopRules, ASpinStartOfAnotherLoopDoesNotStartTheFirstIteration)
{
	FaiLoop l;
	l.add(LoopBeginLabel::create(l.next(1), kInner));
	auto *outer = l.add(SpinStartLabel::create(l.next(1), kLoop));
	EXPECT_FALSE(spin_rules::startsFirstIteration(l.g, *outer));
}

TEST(SpinloopRules, AnUndisturbedFaiLoopMayBlock)
{
	FaiLoop l;
	EXPECT_TRUE(spin_rules::faiZNEConstraintsSat(l.g, *l.checker, *l.zne));
}

TEST(SpinloopRules, AStoreHbBeforeTheFaiDoesNotDisturbIt)
{
	FaiLoop l(true);
	EXPECT_TRUE(spin_rules::faiZNEConstraintsSat(l.g, *l.checker, *l.zne));
}

TEST(SpinloopRules, AStoreInTheIterationKeepsTheLoopRunning)
{
	FaiLoop l;
	l.write(1, l.y);
	auto *zne = l.add(FaiZNESpinEndLabel::create(l.next(1)));
	EXPECT_FALSE(spin_rules::faiZNEConstraintsSat(l.g, *l.checker, *zne));
}

TEST(SpinloopRules, AConcurrentStoreToTheFaiLocationKeepsTheLoopRunning)
{
	FaiLoop l;
	l.spawn(2);
	l.write(2, l.x);
	EXPECT_FALSE(spin_rules::faiZNEConstraintsSat(l.g, *l.checker, *l.zne));
}

TEST(SpinloopRules, ConcurrentFaisAndOtherLocationsDoNotDisturbTheLoop)
{
	FaiLoop l;
	l.spawn(2);
	l.fai(2, l.x);
	l.write(2, l.y);
	EXPECT_TRUE(spin_rules::faiZNEConstraintsSat(l.g, *l.checker, *l.zne));
}

TEST(SpinloopRules, AConcurrentStoreToTheFaiLocationUnblocksTheLoop)
{
	FaiLoop l;
	auto *bLab = l.block();
	l.spawn(2);
	auto *wLab = l.write(2, l.x);
	EXPECT_EQ(spin_rules::faiSpinloopsBrokenBy(l.g, *l.checker, *wLab),
		  std::vector<const FaiZNEBlockLabel *>{bLab});
}

TEST(SpinloopRules, ConcurrentFaisAndOtherLocationsLeaveTheLoopBlocked)
{
	FaiLoop l;
	l.block();
	l.spawn(2);
	auto *fLab = l.fai(2, l.x);
	auto *wLab = l.write(2, l.y);
	EXPECT_TRUE(spin_rules::faiSpinloopsBrokenBy(l.g, *l.checker, *fLab).empty());
	EXPECT_TRUE(spin_rules::faiSpinloopsBrokenBy(l.g, *l.checker, *wLab).empty());
}

TEST(SpinloopRules, AStoreHbBeforeTheFaiLeavesTheLoopBlocked)
{
	FaiLoop l(true);
	l.block();
	const auto *wLab = genmc::cast<WriteLabel>(l.g.getEventLabel(Event(1, 1)));
	EXPECT_TRUE(spin_rules::faiSpinloopsBrokenBy(l.g, *l.checker, *wLab).empty());
}

TEST(SpinloopRules, AThreadThatIsNotBlockedIsNotReconsidered)
{
	FaiLoop l;
	l.spawn(2);
	auto *wLab = l.write(2, l.x);
	EXPECT_TRUE(spin_rules::faiSpinloopsBrokenBy(l.g, *l.checker, *wLab).empty());
}
