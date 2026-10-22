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


#include "genmc/Verification/ExplorationStack.hpp"
#include "genmc/Execution/ModelInterfaces.hpp"
#include "genmc/Execution/DepExecutionGraph.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Verification/Config.hpp"

#include <memory>

auto ExplorationStack::emptyFrame() const -> ExecutionFrame
{
	auto cfg = ExecutionGraph::Config{
		.execState = &state_, .memModel = &model_, .emitNALabels = Config::emitNALabels};
	auto g = depTracking_ ? std::make_unique<DepExecutionGraph>(cfg)
			      : std::make_unique<ExecutionGraph>(cfg);
	return {std::move(g), WorkList(), ChoiceMap()};
}

void ExplorationStack::reset(ExecutionFrame &&frame)
{
	frames_.clear();
	frames_.push_back(std::move(frame));
	top().getGraph().setMemModel(&model_);
	top().getGraph().setState(&state_);
	topChanged();
}

void ExplorationStack::push(ExecutionFrame &&frame)
{
	frames_.push_back(std::move(frame));
	topChanged();
}

auto ExplorationStack::pop() -> bool
{
	if (frames_.empty())
		return false;
	frames_.pop_back();
	if (frames_.empty())
		return false;
	topChanged();
	return true;
}

void ExplorationStack::restrict(Stamp stamp)
{
	top().restrict(stamp);
	/* Cutting the graph bypasses the incremental counter updates */
	topChanged();
}

void ExplorationStack::topChanged() { model_.recomputeCacheCounters(top().getGraph()); }
