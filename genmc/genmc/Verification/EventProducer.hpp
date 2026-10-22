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


#ifndef GENMC_EVENT_PRODUCER_HPP
#define GENMC_EVENT_PRODUCER_HPP

#include "genmc/Execution/Event.hpp"
#include "genmc/Support/ThreadInfo.hpp"
#include "genmc/Verification/ResumePoint.hpp"

#include <memory>
#include <vector>

/**
 * A program the driver explores. The producer runs the program's threads,
 * one step at a time in the order GenMCDriver::scheduleNext() picks, and
 * reports each step's event to the driver through its handle*() methods;
 * the driver answers with the value the step sees. The producer owns no
 * exploration state: everything it needs to rerun a prefix is in the graph.
 *
 * The interpreter is the producer of real programs; tests can script one
 * (see tests/unit/driver), so the exploration can run without LLVM.
 */
class EventProducer {
public:
	EventProducer() = default;
	EventProducer(const EventProducer &) = delete;
	EventProducer(EventProducer &&) = delete;
	auto operator=(const EventProducer &) -> EventProducer & = delete;
	auto operator=(EventProducer &&) -> EventProducer & = delete;
	virtual ~EventProducer() = default;

	/** Runs the program from its start until the driver stops scheduling.
	 * TIS describes the threads of the current graph other than main, which
	 * exist from the start (their creation is replayed from the graph).
	 * With COLLECTDBG, the producer reports debug information (an error is
	 * being replayed for its trace). */
	virtual void runFromStart(const std::vector<ThreadInfo> &tis, bool collectDbg) = 0;

	/** Saves the producer's state at the event at POS, which the current
	 * thread is adding; null if the producer cannot resume (the default) */
	virtual auto saveResumePoint(Event /*pos*/) -> std::shared_ptr<ResumePoint>
	{
		return nullptr;
	}

	/** Restores RP and runs on from it, instead of runFromStart(). Returns
	 * false, having done nothing, if RP cannot be restored on the current
	 * graph (whose threads are TIS and last events LASTIDX) */
	virtual auto resumeFrom(const ResumePoint & /*rp*/, const std::vector<ThreadInfo> & /*tis*/,
				const std::vector<int> & /*lastIdx*/) -> bool
	{
		return false;
	}
};

#endif /* GENMC_EVENT_PRODUCER_HPP */
