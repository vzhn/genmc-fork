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
 * The exploration stack (Verification/State/ExplorationStack.hpp) on its own:
 * frame order, and the binding of every frame to the stack's model.
 */

#include "genmc/Models/ConsistencyChecker.hpp"
#include "genmc/Execution/ExecutionState.hpp"
#include "genmc/Execution/MemoryModel.hpp"
#include "genmc/Verification/State/ExplorationStack.hpp"

#include <gtest/gtest.h>

namespace {

struct Fixture {
	std::unique_ptr<ConsistencyChecker> model = ConsistencyChecker::create(ModelType::SC);
	ExecutionState state;
	ExplorationStack stack{*model, state, false, false};
};

} // namespace

TEST(ExplorationStack, AnEmptyFrameUsesTheStacksModelAndState)
{
	Fixture f;
	auto frame = f.stack.emptyFrame();
	EXPECT_EQ(frame.getGraph().getMemModel(), f.model.get());
	EXPECT_EQ(&frame.getGraph().getState(), &f.state);
	EXPECT_EQ(frame.getGraph().getNumThreads(), 1U);
}

TEST(ExplorationStack, ResetKeepsOnlyTheGivenFrameAndBindsIt)
{
	Fixture f;
	f.stack.reset(f.stack.emptyFrame());
	f.stack.push(f.stack.emptyFrame());
	ASSERT_EQ(f.stack.size(), 2U);

	/* A frame from elsewhere (another worker's graph) */
	Fixture other;
	auto frame = other.stack.emptyFrame();
	const auto *g = frame.graph.get();
	f.stack.reset(std::move(frame));
	EXPECT_EQ(f.stack.size(), 1U);
	EXPECT_EQ(&f.stack.top().getGraph(), g);
	EXPECT_EQ(f.stack.top().getGraph().getMemModel(), f.model.get());
	EXPECT_EQ(&f.stack.top().getGraph().getState(), &f.state);
}

TEST(ExplorationStack, PopReturnsToThePushingFrame)
{
	Fixture f;
	f.stack.reset(f.stack.emptyFrame());
	const auto *bottom = &f.stack.top().getGraph();
	f.stack.push(f.stack.emptyFrame());
	EXPECT_NE(&f.stack.top().getGraph(), bottom);

	EXPECT_TRUE(f.stack.pop());
	EXPECT_EQ(&f.stack.top().getGraph(), bottom);

	/* Popping the last frame ends the exploration */
	EXPECT_FALSE(f.stack.pop());
	EXPECT_TRUE(f.stack.empty());
	EXPECT_FALSE(f.stack.pop());
}
