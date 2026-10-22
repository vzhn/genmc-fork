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


#ifndef GENMC_KEPT_EVENT_RUNS_HPP
#define GENMC_KEPT_EVENT_RUNS_HPP

#include "genmc/Execution/Stamp.hpp"

#include <cstddef>
#include <utility>

/**
 * When a run repeats. A dependency-tracking model's revisit keeps events
 * po-after holes (LKMM, -exhaustive IMM); while a thread fills its holes,
 * three things can happen to a run, and this class records them:
 *
 *  - the thread's kept events had to move (relocation): the run is
 *    repeated from its start, so that everything built during it sees the
 *    new positions;
 *  - an event went before kept ones: the incremental consistency checks
 *    assume each event to come after its thread's others, so the whole
 *    graph is checked, once, when the run ends (deferred); if that check
 *    fails, the run is repeated with the check after each such event
 *    (eager), so that it adds the work it would have added;
 *  - an event could not be aligned with the kept ones: the run is dropped
 *    (the event is not added, and nothing is computed from the graph).
 *
 * A run starts at startRun() and ends at endRun().
 */
class KeptEventRuns {
public:
	/** Which repeat a run that ends needs */
	enum class Repeat : unsigned char { None, AfterRelocation, Eagerly };

	/** A run starts from a graph with maximum stamp MAXSTAMP and WORK
	 * items in its work list */
	void startRun(Stamp maxStamp, size_t work)
	{
		addedBeforeKept_ = false;
		checkPending_ = false;
		dropped_ = false;
		startStamp_ = maxStamp;
		startWork_ = work;
	}

	/** Where a repeat restarts from */
	[[nodiscard]] auto runStartStamp() const -> Stamp { return startStamp_; }
	[[nodiscard]] auto runStartWork() const -> size_t { return startWork_; }

	/** An event went before kept events */
	void addedBeforeKept() { addedBeforeKept_ = true; }
	[[nodiscard]] auto hasAddedBeforeKept() const -> bool { return addedBeforeKept_; }
	/** Whether an event went before kept ones since the last call */
	auto takeAddedBeforeKept() -> bool { return std::exchange(addedBeforeKept_, false); }

	/** Whether checks follow each event before kept ones (else deferred) */
	[[nodiscard]] auto eagerChecks() const -> bool { return eager_; }
	/** The whole graph is to be checked when the run ends */
	void deferCheck() { checkPending_ = true; }
	/** Whether a deferred check is due (once) */
	auto takeDeferredCheck() -> bool { return std::exchange(checkPending_, false); }
	/** The deferred check failed: repeat the run with eager checks */
	void repeatEagerly() { repeatEagerly_ = true; }

	/** The thread's kept events moved: repeat the run */
	void repeatAfterRelocation() { repeatAfterRelocation_ = true; }
	/** How many times the current run was repeated after a relocation */
	[[nodiscard]] auto relocationsInRun() const -> unsigned { return relocations_; }

	/** The run is dropped (no event is added until it ends) */
	void dropRun() { dropped_ = true; }
	[[nodiscard]] auto runDropped() const -> bool { return dropped_; }

	/** The run ends: which repeat it needs. Without one (or when HALTING,
	 * which repeats nothing), the next run is a new one: no relocation
	 * counted, checks deferred again */
	auto endRun(bool halting) -> Repeat
	{
		if (repeatAfterRelocation_ && !halting) {
			repeatAfterRelocation_ = false;
			++relocations_;
			return Repeat::AfterRelocation;
		}
		if (repeatEagerly_ && !halting) {
			repeatEagerly_ = false;
			eager_ = true;
			return Repeat::Eagerly;
		}
		relocations_ = 0;
		eager_ = false;
		return Repeat::None;
	}

private:
	Stamp startStamp_{0};
	size_t startWork_{};
	bool addedBeforeKept_ = false;
	bool checkPending_ = false;
	bool eager_ = false;
	bool repeatEagerly_ = false;
	bool repeatAfterRelocation_ = false;
	unsigned relocations_{};
	bool dropped_ = false;
};

#endif /* GENMC_KEPT_EVENT_RUNS_HPP */
