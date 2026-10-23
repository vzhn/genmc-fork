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
 * The helping-CAS annotation rules (Verification/Rules/HelpingCasRules.hpp)
 * and the in-place revisits of BAM and IPR (Verification/Rules/
 * InPlaceRevisits.hpp) on hand-built RC11 graphs, without a driver.
 */

#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Execution/LoadAnnotation.hpp"
#include "genmc/Execution/MemoryModel.hpp"
#include "genmc/Models/ConsistencyChecker.hpp"
#include "genmc/Support/SExpr.hpp"
#include "genmc/Support/ThreadInfo.hpp"
#include "genmc/Verification/Rules/HelpingCasRules.hpp"
#include "genmc/Verification/Rules/InPlaceRevisits.hpp"

#include <gtest/gtest.h>

#include <memory>

namespace {

/* Threads 1.. on dynamic locations (the initializer's values are not read) */
struct Graph {
	std::unique_ptr<ConsistencyChecker> checker = ConsistencyChecker::create(ModelType::RC11);
	ExecutionGraph g{ExecutionGraph::Config{.memModel = &*checker, .emitNALabels = true}};
	SAddr x = SAddr::createHeap(0, 1, false, false);
	SAddr y = SAddr::createHeap(0, 2, false, false);

	auto spawn() -> int
	{
		auto tid = g.getNumThreads();
		g.addNewThread();
		add(ThreadStartLabel::create(Event(tid, 0), Event(0, 0), nullptr,
					     ThreadInfo{tid, 0, 0, SVal(0), "t"}));
		return tid;
	}

	auto next(int tid) const -> Event { return {tid, g.getThreadSize(tid)}; }

	template <typename LabelT> auto add(std::unique_ptr<LabelT> lab) -> LabelT *
	{
		auto *added = g.add(std::move(lab));
		checker->updateMMViews(added);
		return added;
	}

	/* Adds a write (of kind LabelT) to the end of TID, last in co */
	template <typename LabelT = WriteLabel, typename... Ts>
	auto write(int tid, SAddr addr, SVal val, Ts &&...extra) -> LabelT *
	{
		auto *wLab = g.add(LabelT::create(next(tid), MemOrdering::Relaxed, addr, ASize(4),
						  val, std::forward<Ts>(extra)...));
		wLab->addCo(g.co_max(addr));
		checker->updateMMViews(wLab);
		return wLab;
	}

	/* Adds a read (of kind LabelT) reading from RF to the end of TID */
	template <typename LabelT, typename... Ts>
	auto read(int tid, EventLabel *rf, Ts &&...args) -> LabelT *
	{
		auto *rLab = g.add(LabelT::create(next(tid), std::forward<Ts>(args)...));
		rLab->setRf(rf);
		checker->updateMMViews(rLab);
		return rLab;
	}

	/* A helped CAS on x from 1 to 42 that reads RF (and succeeds if RF wrote 1) */
	auto helped(int tid, WriteLabel *rf) -> HelpedCasReadLabel *
	{
		auto *rLab = read<HelpedCasReadLabel>(tid, rf, MemOrdering::Relaxed,
						      MemOrdering::Relaxed, x, ASize(4), SVal(1),
						      SVal(42), WriteAttr::None, nullptr, std::nullopt);
		if (rf->getVal() == SVal(1))
			write<HelpedCasWriteLabel>(tid, x, SVal(42));
		return rLab;
	}

	auto helping(int tid, SVal swap = SVal(42)) -> HelpingCasLabel *
	{
		return add(HelpingCasLabel::create(next(tid), MemOrdering::Relaxed, x, ASize(4),
						   SVal(1), swap));
	}
};

} // namespace

TEST(HelpingCasRules, AHelpingCasRepeatsASucceededHelpedCasWithTheSameValues)
{
	Graph t;
	auto t1 = t.spawn();
	auto *w = t.write(t1, t.x, SVal(1));
	auto *h = t.helped(t1, w);
	auto t2 = t.spawn();
	EXPECT_EQ(helping_cas::helpedCases(t.g, *t.helping(t2)),
		  std::vector<const HelpedCasReadLabel *>{h});
	EXPECT_TRUE(helping_cas::helpedCases(t.g, *t.helping(t2, SVal(7))).empty());
}

TEST(HelpingCasRules, AFailedHelpedCasIsNotRepeated)
{
	Graph t;
	auto t1 = t.spawn();
	t.helped(t1, t.write(t1, t.x, SVal(0)));
	EXPECT_TRUE(helping_cas::helpedCases(t.g, *t.helping(t.spawn())).empty());
}

TEST(HelpingCasRules, TheHelpingCasMustSeeTheStoresBeforeTheHelpedOne)
{
	Graph t;
	auto t1 = t.spawn();
	auto *w = t.write(t1, t.x, SVal(1));
	t.write(t1, t.y, SVal(1));
	auto *h = t.helped(t1, w);

	/* Another thread does not see them; the helped CAS's own does */
	EXPECT_FALSE(
		helping_cas::seesStoresBeforeHelped(t.g, *t.checker, *h, *t.helping(t.spawn())));
	EXPECT_TRUE(helping_cas::seesStoresBeforeHelped(t.g, *t.checker, *h, *t.helping(t1)));
}

TEST(HelpingCasRules, AThreadStillWaitingForAHelpedCasIsAnError)
{
	Graph t;
	auto t1 = t.spawn();
	t.helping(t1);
	t.write(t.spawn(), t.x, SVal(0)); /* (a store it cannot succeed on) */
	EXPECT_FALSE(helping_cas::annotationError(t.g));
	t.add(HelpedCASBlockLabel::create(t.next(t1)));
	EXPECT_EQ(helping_cas::annotationError(t.g), "Does helped CAS always execute?");
}

TEST(HelpingCasRules, AStoreTheHelpingCasCouldSucceedOnMustBeReadByAHelpedCas)
{
	Graph t;
	auto t1 = t.spawn();
	auto *w = t.write(t1, t.x, SVal(1));
	t.helping(t.spawn());
	EXPECT_EQ(helping_cas::annotationError(t.g), "Unordered store to helping CAS location!");

	/* (A plain read does not count) */
	t.read<ReadLabel>(t.spawn(), w, MemOrdering::Relaxed, t.x, ASize(4), nullptr, std::nullopt);
	EXPECT_EQ(helping_cas::annotationError(t.g), "Unordered store to helping CAS location!");

	t.helped(t.spawn(), w);
	EXPECT_FALSE(helping_cas::annotationError(t.g));
}

namespace {

/*
 * A barrier of N threads on y, initialized by main (to N); thread T
 * of 1..N waits in its round:
 *
 *   R y (= N)  BInc-R y  BInc-W y  BWait-R y
 */
struct Barrier : Graph {
	WriteLabel *init{};
	WriteLabel *last{}; /* the latest increment (or the initialization) */

	explicit Barrier(unsigned n)
	{
		init = write(0, y, SVal(n));
		last = init;
	}

	/* A thread arrives (and waits, if WAITS) */
	auto arrive(bool waits = true) -> BIncFaiWriteLabel *
	{
		auto tid = spawn();
		read<ReadLabel>(tid, init, MemOrdering::Relaxed, y, ASize(4), nullptr,
				std::nullopt);
		read<BIncFaiReadLabel>(tid, last, MemOrdering::Acquire, y, ASize(4), RMWBinOp::Add,
				       SVal(1), WriteAttr::None, nullptr, std::nullopt);
		auto *inc = write<BIncFaiWriteLabel>(tid, y, last->getVal() + SVal(1));
		if (waits)
			read<BWaitReadLabel>(tid, inc, MemOrdering::Acquire, y, ASize(4), nullptr,
					     std::nullopt);
		last = inc;
		return inc;
	}
};

} // namespace

TEST(InPlaceRevisits, TheLastIncrementOfARoundRevisitsTheRoundsWaits)
{
	Barrier b(3);
	auto *i1 = b.arrive();
	auto *i2 = b.arrive();
	auto *i3 = b.arrive();
	auto wait = [&](const EventLabel *inc) { return b.g.po_imm_succ(inc)->getPos(); };
	EXPECT_EQ(in_place::barrierWaits(*i3), (std::vector<Event>{wait(i2), wait(i1)}));
}

TEST(InPlaceRevisits, AnIncrementWithoutItsWaitYetIsSkipped)
{
	Barrier b(2);
	b.arrive(false);
	EXPECT_TRUE(in_place::barrierWaits(*b.arrive()).empty());
}

TEST(InPlaceRevisits, TheRoundEndsAtThePreviousRoundsLastIncrement)
{
	Barrier b(2);
	b.arrive();
	b.arrive();
	auto *i3 = b.arrive();
	auto *i4 = b.arrive();
	EXPECT_EQ(in_place::barrierWaits(*i4),
		  std::vector<Event>{b.g.po_imm_succ(i3)->getPos()});
}

namespace {

/* An annotation on a read: the read only goes on if ASSUMEHOLDS */
auto annotation(bool assumeHolds) -> Annotation
{
	auto expr = assumeHolds ? ConcreteExpr<ModuleVarID>::createTrue()
				: ConcreteExpr<ModuleVarID>::createFalse();
	return {.type = AssumeType::Spinloop,
		.expr = Annotation::ExprVP(static_cast<const Annotation::Expr &>(*expr))};
}

auto annotatedRead(Graph &t, int tid, WriteLabel *rf, bool assumeHolds) -> ReadLabel *
{
	return t.read<ReadLabel>(tid, rf, MemOrdering::Relaxed, t.x, ASize(4), nullptr,
				 std::optional(annotation(assumeHolds)));
}

} // namespace

TEST(InPlaceRevisits, AReadBlocksOnAnAnnotationItsValueFails)
{
	Graph t;
	auto t1 = t.spawn();
	auto *w = t.write(t1, t.x, SVal(1));
	EXPECT_TRUE(in_place::blocksOnAnnotation(*annotatedRead(t, t.spawn(), w, false)));
	EXPECT_FALSE(in_place::blocksOnAnnotation(*annotatedRead(t, t.spawn(), w, true)));
}

TEST(InPlaceRevisits, UnannotatedReadsAndCasesDoNotBlockInPlace)
{
	Graph t;
	auto t1 = t.spawn();
	auto *w = t.write(t1, t.x, SVal(0));
	auto *r = t.read<ReadLabel>(t.spawn(), w, MemOrdering::Relaxed, t.x, ASize(4), nullptr,
				    std::nullopt);
	EXPECT_FALSE(in_place::blocksOnAnnotation(*r));
	/* (Blocked CASes are handled apart) */
	auto *cas = t.read<CasReadLabel>(t.spawn(), w, MemOrdering::Relaxed, MemOrdering::Relaxed,
					 t.x, ASize(4), SVal(1), SVal(42), WriteAttr::None, nullptr,
					 std::optional(annotation(false)));
	EXPECT_FALSE(in_place::blocksOnAnnotation(*cas));
}
