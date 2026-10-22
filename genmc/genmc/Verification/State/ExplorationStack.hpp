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


#ifndef GENMC_EXPLORATION_STACK_HPP
#define GENMC_EXPLORATION_STACK_HPP

#include "genmc/Execution/Stamp.hpp"
#include "genmc/Verification/State/ExecutionFrame.hpp"

#include <vector>

class ExecutionState;
class GraphModel;

/**
 * The depth-first stack of the exploration. Each frame is a graph with the
 * revisits still to explore from it; the top frame is the execution being
 * explored, and a backward revisit pushes the graph it produces.
 *
 * Invariant: every frame's graph uses the stack's memory model and state,
 * and the model's cache counters describe the top frame's graph (they are
 * recomputed whenever the top frame changes other than by adding events).
 */
class ExplorationStack {
public:
	ExplorationStack(GraphModel &model, ExecutionState &state, bool depTracking)
		: model_(model), state_(state), depTracking_(depTracking)
	{}

	/** A frame with the empty graph (only the initializer) */
	[[nodiscard]] auto emptyFrame() const -> ExecutionFrame;

	[[nodiscard]] auto empty() const -> bool { return frames_.empty(); }
	[[nodiscard]] auto size() const -> size_t { return frames_.size(); }

	/** The frame being explored */
	auto top() -> ExecutionFrame & { return frames_.back(); }
	[[nodiscard]] auto top() const -> const ExecutionFrame & { return frames_.back(); }

	/** Starts over from FRAME alone */
	void reset(ExecutionFrame &&frame);

	/** Explores FRAME next; the current top is resumed when it is popped */
	void push(ExecutionFrame &&frame);

	/** Drops the top frame. Returns whether a frame is left */
	auto pop() -> bool;

	/** Removes everything with stamp >= STAMP from the top frame */
	void restrict(Stamp stamp);

private:
	void topChanged();

	GraphModel &model_;
	ExecutionState &state_;
	bool depTracking_;
	std::vector<ExecutionFrame> frames_;
};

#endif /* GENMC_EXPLORATION_STACK_HPP */
