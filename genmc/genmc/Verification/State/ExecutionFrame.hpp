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


#ifndef GENMC_EXECUTION_FRAME_HPP
#define GENMC_EXECUTION_FRAME_HPP

#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Execution/Stamp.hpp"
#include "genmc/Verification/State/ChoiceMap.hpp"
#include "genmc/Verification/State/WorkList.hpp"

#include <memory>

/**
 * One frame of the exploration: an execution graph, the revisits still
 * pending on it, and the rf/co choices already taken. The driver keeps a
 * stack of these; the thread pool moves them between workers.
 */
struct ExecutionFrame {
	ExecutionFrame() = delete;
	ExecutionFrame(std::unique_ptr<ExecutionGraph> g, WorkList &&w, ChoiceMap &&map);

	ExecutionFrame(const ExecutionFrame &) = delete;
	auto operator=(const ExecutionFrame &) -> ExecutionFrame & = delete;
	ExecutionFrame(ExecutionFrame &&) = default;
	auto operator=(ExecutionFrame &&) -> ExecutionFrame & = default;

	~ExecutionFrame();

	/** Returns a reference to the current graph (mutable/const getter pair) */
	// NOLINTNEXTLINE(readability-make-member-function-const)
	auto getGraph() -> ExecutionGraph & { return *graph; }
	auto getGraph() const -> const ExecutionGraph & { return *graph; }

	auto getWorkqueue() -> WorkList & { return workqueue; }
	auto getWorkqueue() const -> const WorkList & { return workqueue; }

	auto getChoiceMap() -> ChoiceMap & { return choices; }
	auto getChoiceMap() const -> const ChoiceMap & { return choices; }

	/** Removes all items with stamp >= STAMP from the execution */
	void restrict(Stamp stamp);

	std::unique_ptr<ExecutionGraph> graph;
	WorkList workqueue;
	ChoiceMap choices;
};

#endif /* GENMC_EXECUTION_FRAME_HPP */
