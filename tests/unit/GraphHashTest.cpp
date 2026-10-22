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

#include <gtest/gtest.h>

#include "StubChecker.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Execution/ExecutionState.hpp"
#include "genmc/Support/ThreadInfo.hpp"
#include "genmc/Verification/GraphHash.hpp"

namespace {

struct HashGraph {
	StubChecker checker;
	ExecutionState st; /* the values read are reconstructed through it */
	ExecutionGraph g{ExecutionGraph::Config{
		.execState = &st, .consChecker = &checker, .emitNALabels = false}};

	/* Creates a thread from (PARENT, IDX) as thread TID (the next one) */
	auto spawn(int parent, int idx) -> int
	{
		auto tid = g.getNumThreads();
		auto tinfo = ThreadInfo{tid, parent, 0, SVal(0), "t"};
		auto *tcLab = genmc::cast<ThreadCreateLabel>(
			g.add(ThreadCreateLabel::create(Event(parent, idx), tinfo)));
		g.addNewThread();
		g.add(ThreadStartLabel::create(Event(tid, 0), tcLab->getPos(), tcLab, tinfo));
		return tid;
	}

	auto write(int tid, SAddr addr, SVal val) -> WriteLabel *
	{
		auto pos = Event(tid, g.getThreadSize(tid));
		auto *wLab = genmc::cast<WriteLabel>(
			g.add(WriteLabel::create(pos, MemOrdering::Relaxed, addr, ASize(4), val)));
		wLab->addCo(g.co_max(addr));
		return wLab;
	}

	auto read(int tid, SAddr addr, EventLabel *rf) -> ReadLabel *
	{
		auto pos = Event(tid, g.getThreadSize(tid));
		return genmc::cast<ReadLabel>(g.add(ReadLabel::create(
			pos, MemOrdering::Relaxed, addr, ASize(4), rf, std::nullopt)));
	}
};

} // namespace

/* main creates A then B; A creates A1 (which writes x), B creates B1 (which
 * reads A1's write). Whether A1 or B1 is created first in a run decides their
 * ids; the description must not. */
TEST(GraphHashTest, ThreadsAreNamedByCreationPath)
{
	auto addr = SAddr::createStatic(0, 1, false, false);

	/* Run 1: A runs first, A1 is thread 3, B1 thread 4 */
	HashGraph g1;
	auto a = g1.spawn(0, 1);
	auto b = g1.spawn(0, 2);
	auto a1 = g1.spawn(a, 1);
	auto b1 = g1.spawn(b, 1);
	auto *w1 = g1.write(a1, addr, SVal(1));
	g1.read(b1, addr, w1);

	/* Run 2: B runs first, B1 is thread 3, A1 thread 4 */
	HashGraph g2;
	auto a_ = g2.spawn(0, 1);
	auto b_ = g2.spawn(0, 2);
	auto b1_ = g2.spawn(b_, 1);
	auto a1_ = g2.spawn(a_, 1);
	auto *w2 = g2.write(a1_, addr, SVal(1));
	g2.read(b1_, addr, w2);

	EXPECT_EQ(canonicalGraphDescription(g1.g), canonicalGraphDescription(g2.g));
	EXPECT_EQ(canonicalGraphHash(g1.g), canonicalGraphHash(g2.g));

	/* Run 3: the same ids as run 2, but B1 writes and A1 reads: a different graph */
	HashGraph g3;
	auto a3 = g3.spawn(0, 1);
	auto b3 = g3.spawn(0, 2);
	auto b13 = g3.spawn(b3, 1);
	auto a13 = g3.spawn(a3, 1);
	auto *w3 = g3.write(b13, addr, SVal(1));
	g3.read(a13, addr, w3);

	EXPECT_NE(canonicalGraphDescription(g1.g), canonicalGraphDescription(g3.g));
	EXPECT_NE(canonicalGraphHash(g1.g), canonicalGraphHash(g3.g));
}

/* A thread's copy of a thread-local carries the thread's id (SAddr), which
 * is named by creation path like the thread (as in the test above, A1 writes
 * its copy and B1 reads it through a pointer) */
TEST(GraphHashTest, ThreadLocalCopiesAreNamedByCreationPath)
{
	auto tl = SAddr::createStatic(0, 8, false, false);

	HashGraph g1;
	auto a = g1.spawn(0, 1);
	auto b = g1.spawn(0, 2);
	auto a1 = g1.spawn(a, 1);
	auto b1 = g1.spawn(b, 1);
	auto *w1 = g1.write(a1, tl.withThread(a1), SVal(1));
	g1.read(b1, tl.withThread(a1), w1);

	HashGraph g2;
	auto a_ = g2.spawn(0, 1);
	auto b_ = g2.spawn(0, 2);
	auto b1_ = g2.spawn(b_, 1);
	auto a1_ = g2.spawn(a_, 1);
	auto *w2 = g2.write(a1_, tl.withThread(a1_), SVal(1));
	g2.read(b1_, tl.withThread(a1_), w2);

	EXPECT_EQ(canonicalGraphDescription(g1.g), canonicalGraphDescription(g2.g));
	EXPECT_EQ(canonicalGraphHash(g1.g), canonicalGraphHash(g2.g));

	/* B1 reading its own copy instead is a different graph */
	HashGraph g3;
	auto a3 = g3.spawn(0, 1);
	auto b3 = g3.spawn(0, 2);
	auto b13 = g3.spawn(b3, 1);
	auto a13 = g3.spawn(a3, 1);
	auto *w3 = g3.write(a13, tl.withThread(b13), SVal(1));
	g3.read(b13, tl.withThread(b13), w3);

	EXPECT_NE(canonicalGraphDescription(g1.g), canonicalGraphDescription(g3.g));
}

/* The rf source and the coherence position tell graphs apart */
TEST(GraphHashTest, RfAndCoherenceAreInTheHash)
{
	auto addr = SAddr::createStatic(0, 1, false, false);

	HashGraph g1;
	auto t1 = g1.spawn(0, 1);
	auto t2 = g1.spawn(0, 2);
	auto *wa = g1.write(t1, addr, SVal(1));
	auto *wb = g1.write(t2, addr, SVal(2));
	g1.read(0, addr, wa);

	HashGraph g2;
	auto u1 = g2.spawn(0, 1);
	auto u2 = g2.spawn(0, 2);
	auto *va = g2.write(u1, addr, SVal(1));
	auto *vb = g2.write(u2, addr, SVal(2));
	g2.read(0, addr, vb);
	EXPECT_NE(canonicalGraphHash(g1.g), canonicalGraphHash(g2.g));

	/* Reading the same write, with the writes in the other coherence order */
	HashGraph g3;
	auto s1 = g3.spawn(0, 1);
	auto s2 = g3.spawn(0, 2);
	auto *xb = g3.write(s2, addr, SVal(2));
	auto *xa = g3.write(s1, addr, SVal(1));
	g3.read(0, addr, xa);
	EXPECT_NE(canonicalGraphHash(g1.g), canonicalGraphHash(g3.g));
	(void)wb;
	(void)va;
	(void)xb;
}

/* The hash is the MurmurHash3 x64 128-bit of the description (a known vector) */
TEST(GraphHashTest, MurmurVector)
{
	auto h = murmurHash128("", 0, 0);
	EXPECT_EQ(h.lo, 0ULL);
	EXPECT_EQ(h.hi, 0ULL);
	auto h2 = murmurHash128("hello", 5, 0);
	EXPECT_NE(h2.lo, 0ULL);
	EXPECT_NE(h2, murmurHash128("hellp", 5, 0));
}
