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
 * Which earlier thread a new one is symmetric to (Verification/Rules/
 * SymmetricThreads.hpp), on hand-built graphs: main creates two threads
 * with the same function and argument, one right after the other.
 */

#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Execution/ExecutionState.hpp"
#include "genmc/Execution/MemoryModel.hpp"
#include "genmc/Models/ConsistencyChecker.hpp"
#include "genmc/Support/ThreadInfo.hpp"
#include "genmc/Verification/Rules/SymmetricThreads.hpp"

#include <gtest/gtest.h>

#include <memory>

namespace {

struct Spawns {
	std::unique_ptr<ConsistencyChecker> checker = ConsistencyChecker::create(ModelType::RC11);
	ExecutionState st;
	ExecutionGraph g{ExecutionGraph::Config{.execState = &st, .memModel = &*checker}};

	static auto info(int id, bool selfUsed, int symm = -1) -> ThreadInfo
	{
		auto ti = ThreadInfo(id, 0, /* funId */ 7, SVal(0), "worker", symm);
		ti.argUsed = false;
		ti.selfUsed = selfUsed;
		return ti;
	}

	/* Main creates thread ID; returns the create */
	auto spawn(const ThreadInfo &ti) -> ThreadCreateLabel *
	{
		auto *tcLab = g.add(ThreadCreateLabel::create(Event(0, g.getThreadSize(0)), ti));
		g.addNewThread();
		g.add(ThreadStartLabel::create(Event(ti.id, 0), tcLab->getPos(), tcLab, ti));
		return tcLab;
	}

	/* The symmetric predecessor of a second worker */
	auto second(bool selfUsed, int symm = -1) -> int
	{
		spawn(info(1, selfUsed));
		auto ti = info(2, selfUsed, symm);
		auto *tcLab = g.add(ThreadCreateLabel::create(Event(0, g.getThreadSize(0)), ti));
		return symmetricPredecessor(g, tcLab, ti);
	}
};

} // namespace

TEST(SymmetricThreads, IdenticalThreadsAreSymmetric)
{
	Spawns s;
	EXPECT_EQ(s.second(false), 1);
}

TEST(SymmetricThreads, AThreadThatReadsItsIdIsNotSymmetric)
{
	Spawns s;
	EXPECT_EQ(s.second(true), -1);
}

TEST(SymmetricThreads, TheUserNamedSymmetryStands)
{
	Spawns s;
	EXPECT_EQ(s.second(true, 1), 1);
}
