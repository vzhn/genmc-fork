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
 * The exploration driven by a scripted producer, without LLVM or the
 * interpreter: litmus tests with known execution counts, and random
 * programs checked against a brute-force axiomatic enumeration.
 */

#include "AxiomaticOracle.hpp"
#include "ScriptedProducer.hpp"

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include <ostream>

namespace scripted {
/* For rapidcheck's counterexamples */
inline void showValue(const Program &p, std::ostream &os)
{
	static constexpr const char *kOrd[] = {"na", "rlx", "", "acq", "rel", "acqrel", "sc"};
	for (auto t = 0U; t < p.threads.size(); t++) {
		os << (t ? " || " : "");
		for (auto i = 0U; i < p.threads[t].size(); i++) {
			const auto &op = p.threads[t][i];
			os << (i ? "; " : "");
			if (op.kind == OpKind::Load)
				os << "r=v" << op.var;
			else if (op.kind == OpKind::Store)
				os << "v" << op.var << "=" << op.val;
			else if (op.kind == OpKind::FetchAdd)
				os << (op.blind ? "blindfai(v" : "r=fai(v") << op.var << "," << op.val << ")";
			else if (op.kind == OpKind::CompareSwap)
				os << "r=cas(v" << op.var << "," << op.expected << "->" << op.val << ")";
			else
				os << "fence";
			os << "." << kOrd[static_cast<int>(op.ord)];
		}
	}
}
} // namespace scripted

using namespace scripted;

namespace {

constexpr auto rlx = MemOrdering::Relaxed;
constexpr auto acq = MemOrdering::Acquire;
constexpr auto rel = MemOrdering::Release;
constexpr auto sc = MemOrdering::SequentiallyConsistent;
constexpr unsigned X = 0;
constexpr unsigned Y = 1;

auto outcome(std::initializer_list<std::vector<std::uint32_t>> threads) -> Outcome
{
	return {threads};
}

/* The values read in OUTCOMES (without the final memory) */
auto reads(const Exploration &r) -> std::set<Outcome>
{
	std::set<Outcome> res;
	for (auto o : r.outcomes) {
		o.pop_back();
		res.insert(o);
	}
	return res;
}

/* The driver explores what the axioms allow: as many graphs, same outcomes */
void expectMatchesOracle(const Program &p, ModelType model)
{
	auto got = explore(p, model);
	auto want = oracle::enumerate(p, model);
	EXPECT_FALSE(got.error);
	EXPECT_EQ(got.explored, want.graphs);
	EXPECT_EQ(got.outcomes, want.outcomes);
}

} // namespace

TEST(ScriptedDriver, SingleThreadReadsItsOwnWrite)
{
	Program p{.numVars = 1, .threads = {{st(X, 7), ld(X)}}};
	auto r = explore(p, ModelType::RC11);
	EXPECT_EQ(r.explored, 1U);
	EXPECT_FALSE(r.error);
	EXPECT_EQ(reads(r), std::set<Outcome>{outcome({{7}})});
}

TEST(ScriptedDriver, StoreBufferingRelaxed)
{
	Program p{.numVars = 2, .threads = {{st(X, 1), ld(Y)}, {st(Y, 1), ld(X)}}};
	auto r = explore(p, ModelType::RC11);
	EXPECT_EQ(r.explored, 4U);
	EXPECT_TRUE(reads(r).contains(outcome({{0}, {0}})));
	expectMatchesOracle(p, ModelType::RC11);
	expectMatchesOracle(p, ModelType::SC);
}

TEST(ScriptedDriver, ReadThenWriteAgainstWriteThenRead)
{
	Program p{.numVars = 1, .threads = {{ld(X), st(X, 1)}, {st(X, 1), ld(X)}}};
	expectMatchesOracle(p, ModelType::SC);
}

TEST(ScriptedDriver, StoreBufferingSCAccessesForbidBothZero)
{
	Program p{.numVars = 2,
		  .threads = {{st(X, 1, sc), ld(Y, sc)}, {st(Y, 1, sc), ld(X, sc)}}};
	auto r = explore(p, ModelType::RC11);
	EXPECT_EQ(r.explored, 3U);
	EXPECT_FALSE(reads(r).contains(outcome({{0}, {0}})));
}

TEST(ScriptedDriver, StoreBufferingFencesForbidBothZero)
{
	Program p{.numVars = 2,
		  .threads = {{st(X, 1), fence(), ld(Y)}, {st(Y, 1), fence(), ld(X)}}};
	auto r = explore(p, ModelType::RC11);
	EXPECT_EQ(r.explored, 3U);
	EXPECT_FALSE(reads(r).contains(outcome({{0}, {0}})));
}

TEST(ScriptedDriver, StoreBufferingTSOAllowsBothZero)
{
	Program p{.numVars = 2, .threads = {{st(X, 1), ld(Y)}, {st(Y, 1), ld(X)}}};
	auto r = explore(p, ModelType::TSO);
	EXPECT_EQ(r.explored, 4U);
	EXPECT_TRUE(reads(r).contains(outcome({{0}, {0}})));
}

TEST(ScriptedDriver, MessagePassingRelaxedAllowsStaleData)
{
	Program p{.numVars = 2, .threads = {{st(X, 1), st(Y, 1)}, {ld(Y), ld(X)}}};
	auto r = explore(p, ModelType::RC11);
	EXPECT_EQ(r.explored, 4U);
	EXPECT_TRUE(reads(r).contains(outcome({{}, {1, 0}})));
	expectMatchesOracle(p, ModelType::RC11);
}

TEST(ScriptedDriver, MessagePassingReleaseAcquireForbidsStaleData)
{
	Program p{.numVars = 2, .threads = {{st(X, 1), st(Y, 1, rel)}, {ld(Y, acq), ld(X)}}};
	auto r = explore(p, ModelType::RC11);
	EXPECT_EQ(r.explored, 3U);
	EXPECT_FALSE(reads(r).contains(outcome({{}, {1, 0}})));
}

TEST(ScriptedDriver, LoadBufferingDependsOnTheModel)
{
	Program p{.numVars = 2, .threads = {{ld(X), st(Y, 1)}, {ld(Y), st(X, 1)}}};
	auto rc11 = explore(p, ModelType::RC11);
	EXPECT_EQ(rc11.explored, 3U);
	EXPECT_FALSE(reads(rc11).contains(outcome({{1}, {1}})));
	expectMatchesOracle(p, ModelType::RC11);

	/* No dependencies: IMM allows the cycle */
	auto imm = explore(p, ModelType::IMM);
	EXPECT_EQ(imm.explored, 4U);
	EXPECT_TRUE(reads(imm).contains(outcome({{1}, {1}})));
}

TEST(ScriptedDriver, TwoPlusTwoWritesDependsOnTheModel)
{
	/* Each co order of each location is a separate graph */
	Program p{.numVars = 2, .threads = {{st(X, 1), st(Y, 2)}, {st(Y, 1), st(X, 2)}}};
	EXPECT_EQ(explore(p, ModelType::RC11).explored, 4U);
	EXPECT_EQ(explore(p, ModelType::SC).explored, 3U);
	expectMatchesOracle(p, ModelType::RC11);
	expectMatchesOracle(p, ModelType::SC);
}

TEST(ScriptedDriver, CoherenceOfReadReadForbidsGoingBack)
{
	/* CoRR: a thread that saw the second write cannot see the first after */
	Program p{.numVars = 1, .threads = {{st(X, 1), st(X, 2)}, {ld(X), ld(X)}}};
	auto r = explore(p, ModelType::RC11);
	EXPECT_FALSE(reads(r).contains(outcome({{}, {2, 1}})));
	EXPECT_FALSE(reads(r).contains(outcome({{}, {2, 0}})));
	expectMatchesOracle(p, ModelType::RC11);
}

TEST(ScriptedDriver, ConcurrentIncrementsAreNeverLost)
{
	/* Two increments: each order is one execution, and the counter ends at 2 */
	Program p{.numVars = 1, .threads = {{fai(X, 1)}, {fai(X, 1)}, {}}};
	p.threads[2] = {}; /* (an empty third thread) */
	auto r = explore(p, ModelType::RC11);
	EXPECT_EQ(r.explored, 2U);
	EXPECT_EQ(reads(r), (std::set<Outcome>{outcome({{0}, {1}, {}}), outcome({{1}, {0}, {}})}));
	expectMatchesOracle(p, ModelType::RC11);
	expectMatchesOracle(p, ModelType::SC);
}

TEST(ScriptedDriver, AReadAfterIncrementsSeesEveryPrefix)
{
	Program p{.numVars = 1, .threads = {{fai(X, 1)}, {fai(X, 2)}, {ld(X)}}};
	auto r = explore(p, ModelType::RC11);
	for (auto v : {0U, 1U, 2U, 3U})
		EXPECT_TRUE(std::ranges::any_of(reads(r), [v](auto &o) { return o[2] == std::vector{v}; }))
			<< "the reader never sees " << v;
	expectMatchesOracle(p, ModelType::RC11);
	expectMatchesOracle(p, ModelType::SC);
}

TEST(ScriptedDriver, AnIncrementRevisitedByAPlainStore)
{
	/* The store can go before the increment (it reads 5) or after it */
	Program p{.numVars = 1, .threads = {{fai(X, 1), ld(X)}, {st(X, 5)}}};
	expectMatchesOracle(p, ModelType::RC11);
	expectMatchesOracle(p, ModelType::SC);
}

TEST(ScriptedDriver, OnlyOneOfTwoLocksSucceeds)
{
	/* Two CASes from 0 to their thread's id: exactly one wins */
	Program p{.numVars = 1, .threads = {{cas(X, 0, 1)}, {cas(X, 0, 2)}}};
	auto r = explore(p, ModelType::RC11);
	EXPECT_EQ(r.explored, 2U);
	EXPECT_EQ(reads(r), (std::set<Outcome>{outcome({{0}, {1}}), outcome({{2}, {0}})}));
	expectMatchesOracle(p, ModelType::RC11);
	expectMatchesOracle(p, ModelType::SC);
}

TEST(ScriptedDriver, AFailedCasWritesNothing)
{
	/* The CAS fails reading 5 (no write), succeeds reading 0 */
	Program p{.numVars = 1, .threads = {{cas(X, 0, 1), ld(X)}, {st(X, 5)}}};
	expectMatchesOracle(p, ModelType::RC11);
	expectMatchesOracle(p, ModelType::SC);
}

TEST(ScriptedDriver, CasAfterAnIncrement)
{
	Program p{.numVars = 1, .threads = {{fai(X, 1)}, {cas(X, 1, 7)}, {ld(X)}}};
	expectMatchesOracle(p, ModelType::RC11);
	expectMatchesOracle(p, ModelType::SC);
}

/*
 * Random programs: up to three threads of up to three accesses to two
 * variables. The driver explores exactly the graphs the axioms allow and
 * sees exactly their outcomes.
 */
namespace {

auto genOp(bool relaxedOnly) -> rc::Gen<Op>
{
	return rc::gen::apply(
		[relaxedOnly](unsigned kind, unsigned var, unsigned val, unsigned ord) {
			static constexpr MemOrdering kOrds[] = {rlx, acq, rel, sc};
			auto o = relaxedOnly ? rlx : kOrds[ord];
			if (kind == 0)
				return ld(var, o == rel ? rlx : o);
			if (kind == 1)
				return st(var, val, o == acq ? rlx : o);
			if (kind == 2)
				return fai(var, val, o);
			return cas(var, val - 1, val, o); /* expects 0..2: may fail or succeed */
		},
		rc::gen::inRange(0U, 4U), rc::gen::inRange(0U, 2U), rc::gen::inRange(1U, 4U),
		rc::gen::inRange(0U, 4U));
}

auto genProgram(bool relaxedOnly) -> rc::Gen<Program>
{
	auto genThread = rc::gen::mapcat(rc::gen::inRange<std::size_t>(1, 4), [=](std::size_t n) {
		return rc::gen::container<std::vector<Op>>(n, genOp(relaxedOnly));
	});
	auto genThreads = rc::gen::mapcat(rc::gen::inRange<std::size_t>(1, 4), [=](std::size_t n) {
		return rc::gen::container<std::vector<std::vector<Op>>>(n, genThread);
	});
	return rc::gen::map(genThreads, [](std::vector<std::vector<Op>> ts) {
		return Program{.numVars = 2, .threads = std::move(ts)};
	});
}

auto checkAgainstOracle(const Program &p, ModelType model) -> void
{
	auto got = explore(p, model);
	auto want = oracle::enumerate(p, model);
	RC_ASSERT(!got.error);
	RC_ASSERT(got.explored == want.graphs);
	RC_ASSERT(got.outcomes == want.outcomes);
}

} // namespace

RC_GTEST_PROP(ScriptedDriverOracle, SCExploresExactlyTheConsistentGraphs, ())
{
	checkAgainstOracle(*genProgram(false), ModelType::SC);
}

RC_GTEST_PROP(ScriptedDriverOracle, RC11RelaxedExploresExactlyTheConsistentGraphs, ())
{
	checkAgainstOracle(*genProgram(true), ModelType::RC11);
}

/*
 * The reductions lose no behaviour: with a reduction on, the outcomes
 * (values read and final memory) are those without it, in no more
 * executions. A run the reduction declines (it reports that the program is
 * outside its conditions) says nothing and is discarded.
 */
namespace {

/* Up to three threads of identical blind increments of X (the setting of
 * -blind-rmw-reduction), reads of X, and relaxed accesses of Y; now and
 * then a write to X the reduction does not cover */
auto genBlindProgram() -> rc::Gen<Program>
{
	auto genOp = rc::gen::apply(
		[](unsigned kind, unsigned val) {
			switch (kind) {
			case 0:
			case 1:
			case 2:
				return blindFai(X, 1);
			case 3:
				return ld(X);
			case 4:
				return ld(Y);
			case 5:
				return st(Y, val);
			default:
				return st(X, val);
			}
		},
		rc::gen::inRange(0U, 7U), rc::gen::inRange(1U, 3U));
	auto genThread = rc::gen::mapcat(rc::gen::inRange<std::size_t>(1, 4), [=](std::size_t n) {
		return rc::gen::container<std::vector<Op>>(n, genOp);
	});
	return rc::gen::map(rc::gen::mapcat(rc::gen::inRange<std::size_t>(1, 4),
					    [=](std::size_t n) {
						    return rc::gen::container<std::vector<std::vector<Op>>>(
							    n, genThread);
					    }),
			    [](std::vector<std::vector<Op>> ts) {
				    return Program{.numVars = 2, .threads = std::move(ts)};
			    });
}

/* A thread body, repeated: two to three copies, and maybe one other thread */
auto genSymmetricProgram() -> rc::Gen<Program>
{
	return rc::gen::apply(
		[](std::vector<Op> body, std::size_t copies, std::vector<Op> other, bool hasOther) {
			Program p{.numVars = 2};
			for (auto i = 0U; i < copies; i++)
				p.threads.push_back(body);
			if (hasOther)
				p.threads.push_back(other);
			return p;
		},
		rc::gen::container<std::vector<Op>>(2, genOp(true)), rc::gen::inRange<std::size_t>(2, 4),
		rc::gen::container<std::vector<Op>>(2, genOp(true)), rc::gen::arbitrary<bool>());
}

/* OUTCOMES with the rows of identical threads in a canonical order */
auto upToSymmetry(const Program &p, const std::set<Outcome> &outcomes) -> std::set<Outcome>
{
	std::set<Outcome> res;
	for (auto o : outcomes) {
		for (auto i = 0U; i < p.threads.size(); i++) {
			std::vector<std::size_t> same;
			for (auto j = 0U; j < p.threads.size(); j++)
				if (p.threads[j] == p.threads[i])
					same.push_back(j);
			if (same.front() != i)
				continue;
			std::vector<std::vector<std::uint32_t>> rows;
			for (auto j : same)
				rows.push_back(o[j]);
			std::ranges::sort(rows);
			for (auto k = 0U; k < same.size(); k++)
				o[same[k]] = rows[k];
		}
		res.insert(o);
	}
	return res;
}

} // namespace

RC_GTEST_PROP(ScriptedDriverReductions, BlindRMWReductionKeepsTheOutcomes, ())
{
	auto p = *genBlindProgram();
	auto off = explore(p, ModelType::RC11);
	auto on = explore(p, ModelType::RC11, {.blindRMWReduction = true});
	RC_CLASSIFY(on.error, "declined");
	RC_PRE(!on.error);
	RC_ASSERT(!off.error);
	RC_ASSERT(on.outcomes == off.outcomes);
	RC_ASSERT(on.explored <= off.explored);
	RC_CLASSIFY(on.explored < off.explored, "reduced");
}

RC_GTEST_PROP(ScriptedDriverReductions, SymmetryReductionKeepsTheOutcomes, ())
{
	auto p = *genSymmetricProgram();
	auto off = explore(p, ModelType::RC11);
	auto on = explore(p, ModelType::RC11, {.symmetryReduction = true});
	RC_PRE(!on.error);
	RC_ASSERT(!off.error);
	RC_ASSERT(upToSymmetry(p, on.outcomes) == upToSymmetry(p, off.outcomes));
	RC_ASSERT(on.explored <= off.explored);
	RC_CLASSIFY(on.explored < off.explored, "reduced");
}

TEST(ScriptedDriverReductions, BlindIncrementsAreExploredInOneOrder)
{
	/* Two identical blind increments commute: one execution, not two */
	Program p{.numVars = 1, .threads = {{blindFai(X, 1)}, {blindFai(X, 1)}}};
	auto off = explore(p, ModelType::RC11);
	auto on = explore(p, ModelType::RC11, {.blindRMWReduction = true});
	EXPECT_EQ(off.explored, 2U);
	EXPECT_EQ(on.explored, 1U);
	EXPECT_EQ(on.outcomes, off.outcomes);
}

/*
 * The default exploration against the reference (-exhaustive, which takes
 * every revisit and dedups the graphs): the same executions, under every
 * model the scripted programs can use without event keys. No oracle is
 * needed, so this covers TSO and RA too.
 */
namespace {

auto checkAgainstExhaustive(const Program &p, ModelType model) -> void
{
	auto def = explore(p, model);
	auto ref = explore(p, model, {.exhaustive = true});
	RC_ASSERT(!def.error);
	RC_ASSERT(!ref.error);
	RC_ASSERT(def.explored == ref.explored);
	RC_ASSERT(def.outcomes == ref.outcomes);
}

} // namespace

RC_GTEST_PROP(ScriptedDriverExhaustive, SCMatchesTheReference, ())
{
	checkAgainstExhaustive(*genProgram(false), ModelType::SC);
}

RC_GTEST_PROP(ScriptedDriverExhaustive, TSOMatchesTheReference, ())
{
	checkAgainstExhaustive(*genProgram(false), ModelType::TSO);
}

RC_GTEST_PROP(ScriptedDriverExhaustive, RAMatchesTheReference, ())
{
	checkAgainstExhaustive(*genProgram(false), ModelType::RA);
}

RC_GTEST_PROP(ScriptedDriverExhaustive, RC11MatchesTheReference, ())
{
	checkAgainstExhaustive(*genProgram(false), ModelType::RC11);
}
