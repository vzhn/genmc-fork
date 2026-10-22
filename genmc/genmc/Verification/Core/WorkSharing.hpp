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


#ifndef GENMC_WORK_SHARING_HPP
#define GENMC_WORK_SHARING_HPP

#include "genmc/Verification/State/ExecutionFrame.hpp"

#include <memory>

/**
 * What a driver needs from the workers it shares the exploration with:
 * a way to hand off a frame, to see whether others are starving, and to
 * stop everyone. The core library only knows this interface; the thread
 * pool that implements it lives with the interpreter.
 */
class WorkSharing {
public:
	WorkSharing() = default;
	WorkSharing(const WorkSharing &) = delete;
	WorkSharing(WorkSharing &&) = delete;
	auto operator=(const WorkSharing &) -> WorkSharing & = delete;
	auto operator=(WorkSharing &&) -> WorkSharing & = delete;
	virtual ~WorkSharing() = default;

	/** Hands FRAME to some worker for exploration */
	virtual void submit(std::unique_ptr<ExecutionFrame> frame) = 0;

	/** The number of frames submitted and not yet finished */
	virtual auto getRemainingTasks() -> unsigned = 0;

	/** The number of workers sharing the exploration */
	[[nodiscard]] virtual auto getNumWorkers() const -> unsigned int = 0;

	/** Whether some worker asked everyone to stop */
	[[nodiscard]] virtual auto shouldHalt() const -> bool = 0;

	/** Asks every worker to stop */
	virtual void halt() = 0;
};

#endif /* GENMC_WORK_SHARING_HPP */
