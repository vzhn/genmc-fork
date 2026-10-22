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
				os << "r=fai(v" << op.var << "," << op.val << ")";
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
	EXPECT_EQ(r.outcomes, std::set<Outcome>{outcome({{7}})});
}

TEST(ScriptedDriver, StoreBufferingRelaxed)
{
	Program p{.numVars = 2, .threads = {{st(X, 1), ld(Y)}, {st(Y, 1), ld(X)}}};
	auto r = explore(p, ModelType::RC11);
	EXPECT_EQ(r.explored, 4U);
	EXPECT_TRUE(r.outcomes.contains(outcome({{0}, {0}})));
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
	EXPECT_FALSE(r.outcomes.contains(outcome({{0}, {0}})));
}

TEST(ScriptedDriver, StoreBufferingFencesForbidBothZero)
{
	Program p{.numVars = 2,
		  .threads = {{st(X, 1), fence(), ld(Y)}, {st(Y, 1), fence(), ld(X)}}};
	auto r = explore(p, ModelType::RC11);
	EXPECT_EQ(r.explored, 3U);
	EXPECT_FALSE(r.outcomes.contains(outcome({{0}, {0}})));
}

TEST(ScriptedDriver, StoreBufferingTSOAllowsBothZero)
{
	Program p{.numVars = 2, .threads = {{st(X, 1), ld(Y)}, {st(Y, 1), ld(X)}}};
	auto r = explore(p, ModelType::TSO);
	EXPECT_EQ(r.explored, 4U);
	EXPECT_TRUE(r.outcomes.contains(outcome({{0}, {0}})));
}

TEST(ScriptedDriver, MessagePassingRelaxedAllowsStaleData)
{
	Program p{.numVars = 2, .threads = {{st(X, 1), st(Y, 1)}, {ld(Y), ld(X)}}};
	auto r = explore(p, ModelType::RC11);
	EXPECT_EQ(r.explored, 4U);
	EXPECT_TRUE(r.outcomes.contains(outcome({{}, {1, 0}})));
	expectMatchesOracle(p, ModelType::RC11);
}

TEST(ScriptedDriver, MessagePassingReleaseAcquireForbidsStaleData)
{
	Program p{.numVars = 2, .threads = {{st(X, 1), st(Y, 1, rel)}, {ld(Y, acq), ld(X)}}};
	auto r = explore(p, ModelType::RC11);
	EXPECT_EQ(r.explored, 3U);
	EXPECT_FALSE(r.outcomes.contains(outcome({{}, {1, 0}})));
}

TEST(ScriptedDriver, LoadBufferingDependsOnTheModel)
{
	Program p{.numVars = 2, .threads = {{ld(X), st(Y, 1)}, {ld(Y), st(X, 1)}}};
	auto rc11 = explore(p, ModelType::RC11);
	EXPECT_EQ(rc11.explored, 3U);
	EXPECT_FALSE(rc11.outcomes.contains(outcome({{1}, {1}})));
	expectMatchesOracle(p, ModelType::RC11);

	/* No dependencies: IMM allows the cycle */
	auto imm = explore(p, ModelType::IMM);
	EXPECT_EQ(imm.explored, 4U);
	EXPECT_TRUE(imm.outcomes.contains(outcome({{1}, {1}})));
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
	EXPECT_FALSE(r.outcomes.contains(outcome({{}, {2, 1}})));
	EXPECT_FALSE(r.outcomes.contains(outcome({{}, {2, 0}})));
	expectMatchesOracle(p, ModelType::RC11);
}

TEST(ScriptedDriver, ConcurrentIncrementsAreNeverLost)
{
	/* Two increments: each order is one execution, and the counter ends at 2 */
	Program p{.numVars = 1, .threads = {{fai(X, 1)}, {fai(X, 1)}, {}}};
	p.threads[2] = {}; /* (an empty third thread) */
	auto r = explore(p, ModelType::RC11);
	EXPECT_EQ(r.explored, 2U);
	EXPECT_EQ(r.outcomes, (std::set<Outcome>{outcome({{0}, {1}, {}}), outcome({{1}, {0}, {}})}));
	expectMatchesOracle(p, ModelType::RC11);
	expectMatchesOracle(p, ModelType::SC);
}

TEST(ScriptedDriver, AReadAfterIncrementsSeesEveryPrefix)
{
	Program p{.numVars = 1, .threads = {{fai(X, 1)}, {fai(X, 2)}, {ld(X)}}};
	auto r = explore(p, ModelType::RC11);
	for (auto v : {0U, 1U, 2U, 3U})
		EXPECT_TRUE(std::ranges::any_of(r.outcomes, [v](auto &o) { return o[2] == std::vector{v}; }))
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
			return fai(var, val, o);
		},
		rc::gen::inRange(0U, 3U), rc::gen::inRange(0U, 2U), rc::gen::inRange(1U, 4U),
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
