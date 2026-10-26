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


#ifndef GENMC_ERROR_REPLAY_HPP
#define GENMC_ERROR_REPLAY_HPP

#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Support/Error.hpp"
#include "genmc/Support/Event.hpp"
#include "genmc/Support/VerificationError.hpp"
#include "genmc/Verification/Report/DebugInfo.hpp"
#include "genmc/Verification/State/Scheduler.hpp"

#include <optional>

/**
 * An error is reported after one more run of its graph, which collects the
 * source information of the events up to it (the error replay). Between
 * start() and stop(), the scheduler replays the graph up to the target,
 * the run records debug information, and the error waits here until the
 * run reaches the target (see -check-invariants, I2).
 */
class ErrorReplay {
public:
	explicit ErrorReplay(Scheduler &scheduler) : scheduler_(scheduler) {}

	/** The next run replays the graph up to TARGET, then reports DETAILS */
	void start(Event target, const ErrorDetails &details)
	{
		scheduler_.setErrorReplayEvent(target);
		scheduler_.setErrorReplayPartner(details.racyLab ? std::optional(details.racyLab->getPos())
								 : std::nullopt);
		buffered_ = details;
	}

	/** Whether a replay is running */
	[[nodiscard]] auto active() const -> bool { return scheduler_.inErrorReplay(); }

	/** The event the replay runs up to */
	[[nodiscard]] auto target() const -> std::optional<Event>
	{
		return scheduler_.getErrorReplayEvent();
	}

	/** The error the replay reports */
	[[nodiscard]] auto buffered() const -> const std::optional<ErrorDetails> & { return buffered_; }

	/** Whether the replay reached its target at POS */
	[[nodiscard]] auto reachedTarget(Event pos) const -> bool
	{
		return buffered_ && scheduler_.isErrorReplayEvent(pos);
	}

	/** The source information of the event at POS (once per event) */
	void record(Event pos, const EventDbgInfo &dbg)
	{
		VERIFY(!dbgInfo_.contains(pos));
		dbgInfo_[pos] = dbg;
	}

	/** The source information collected so far */
	[[nodiscard]] auto dbgInfo() const -> const GraphDbgInfo & { return dbgInfo_; }
	auto dbgInfo() -> GraphDbgInfo & { return dbgInfo_; }

	/** The replay is over (reported, or replaced): forget it */
	void stop()
	{
		scheduler_.setErrorReplayEvent(std::nullopt);
		scheduler_.setErrorReplayPartner(std::nullopt);
		dbgInfo_.clear();
		buffered_ = std::nullopt;
	}

private:
	Scheduler &scheduler_;
	std::optional<ErrorDetails> buffered_;
	GraphDbgInfo dbgInfo_;
};

#endif /* GENMC_ERROR_REPLAY_HPP */
