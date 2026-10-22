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

#ifndef GENMC_SCHEDULER_HPP
#define GENMC_SCHEDULER_HPP

#include "genmc/ADT/Trie.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Support/ActionEnums.hpp"

#include <compare>
#include <optional>
#include <random>
#include <utility>
#include <vector>

struct Config;

/** Cache keys are of the form <value, fails_spuriously>. */
struct CacheKey {
	SVal val;
	bool spuriouslyFails = false;

	auto operator<=>(const CacheKey &other) const -> std::weak_ordering
	{
		if (val.ult(other.val))
			return std::weak_ordering::less;
		if (other.val.ult(val))
			return std::weak_ordering::greater;
		return spuriouslyFails <=> other.spuriouslyFails;
	}
	auto operator==(const CacheKey &other) const -> bool = default;
};

class Scheduler {
public:
	Scheduler() = delete;
	Scheduler(const Scheduler &) = delete;
	Scheduler(Scheduler &&) = default;
	auto operator=(const Scheduler &) -> Scheduler & = delete;
	auto operator=(Scheduler &&) -> Scheduler & = default;
	virtual ~Scheduler() = default;

	static auto create(const Config *config) -> std::unique_ptr<Scheduler>;

	/** Resets the options for a new execution (to be called at the
	 * beginning of each one), including the schedule that drives the
	 * runtime through the graph's events: a replayed execution through all
	 * of them, a resumed one through those its threads have not reached,
	 * i.e. the holes before kept events and the kept events themselves */
	void resetExplorationOptions(const ExecutionGraph &g);

	/** The events the runtime is still to be driven through, latest first */
	[[nodiscard]] auto getReplaySchedule() const -> const std::vector<Event> &
	{
		return replaySchedule_;
	}

	/** Returns the next thread to run according to the specified policy.
	 * If every thread is blocked or done, a blocked read may be retried:
	 * its thread is returned, and takeReadToUnblock() tells which read to
	 * unblock before the thread runs (the scheduler does not change G) */
	[[nodiscard]] auto schedule(const ExecutionGraph &g, std::span<Action> runnable)
		-> std::optional<int>;

	/** The read the last schedule chose to retry, once (see schedule()) */
	auto takeReadToUnblock() -> std::optional<Event> { return std::exchange(readToUnblock_, {}); }

	/** Prioritizes thread of POS */
	void prioritize(Event pos) { threadPrios_ = {pos}; }

	/** Adds LAB to the cache. Should be called before LAB is added to G. */
	void cacheEventLabel(const ExecutionGraph &g, const EventLabel *lab);

	/** The thread whose cached labels to add next; nullopt if the execution
	 * is full. Like schedule(), it may choose a blocked read to retry (see
	 * takeReadToUnblock()), which must be unblocked before
	 * retrieveFromCache() */
	[[nodiscard]] auto nextThreadFromCache(const ExecutionGraph &g) -> std::optional<int>;

	/** Opt: Retrieves the next labels to add for THREAD from the cache.
	 * Returns nullptr if no cached info exists. */
	auto retrieveFromCache(const ExecutionGraph &g, int thread)
		-> std::vector<std::unique_ptr<EventLabel>> *;

	/** Opt: Whether the exploration should try to repair R */
	[[nodiscard]] auto isRescheduledRead(Event r) const -> bool
	{
		return readToReschedule_ == r;
	}

	/** Opt: Sets R as a read to be repaired */
	void setRescheduledRead(Event r) { readToReschedule_ = r; }

	/** Opt: Whether we are re-executing up to E (e.g., for error reporting) */
	[[nodiscard]] auto isErrorReplayEvent(Event e) const -> bool
	{
		return eventToReexecute_ == e;
	}

	/** Opt: Whether there is an event to re-execute */
	[[nodiscard]] auto inErrorReplay() const -> bool { return eventToReexecute_.has_value(); }

	/** Opt: The event an error replay re-executes up to (if any) */
	[[nodiscard]] auto getErrorReplayEvent() const -> std::optional<Event>
	{
		return eventToReexecute_;
	}

	/** Opt: Sets E as an event to be re-executed */
	void setErrorReplayEvent(std::optional<Event> e) { eventToReexecute_ = e; }

protected:
	Scheduler(const Config *config) : conf_(config) {}

	[[nodiscard]] auto getConf() const -> const Config * { return conf_; }

private:
	using ValuePrefixT = std::unordered_map<
		std::pair<unsigned int, unsigned int>, // fun_id, tid
		Trie<std::vector<CacheKey>, std::vector<std::unique_ptr<EventLabel>>>,
		PairHasher<unsigned int, unsigned int>>;

	/** Opt: Checks whether SEQ has been seen before for <FUN_ID, TID> and
	 * if so returns its successors. Returns nullptr otherwise. */
	auto retrieveCachedSuccessors(std::pair<unsigned int, unsigned int> key,
				      const std::vector<CacheKey> &seq)
		-> std::vector<std::unique_ptr<EventLabel>> *
	{
		return seenPrefixes[key].lookup(seq);
	}

	/** If there are more events to replay, schedules according to those */
	auto scheduleReplay(const ExecutionGraph &g, std::span<Action> runnable)
		-> std::optional<int>;

	/** Schedule according to the current prioritization scheme (if any) */
	[[nodiscard]] auto schedulePrioritized(const ExecutionGraph &g) -> std::optional<int>;

	/** Opt: Reschedules opt-blocked reads */
	[[nodiscard]] auto rescheduleReads(const ExecutionGraph &g) -> std::optional<int>;

	/** Schedules according to the selected policy */
	[[nodiscard]] virtual auto schedulePolicy(const ExecutionGraph &g,
						  std::span<Action> runnable)
		-> std::optional<int> = 0;

	/** Scheduling configuration */
	const Config *conf_{};

	/** The schedule for replays (porf-linearization) */
	std::vector<Event> replaySchedule_;

	/** Opt: Whether we need to re-execute up to a given event (error reporting) */
	std::optional<Event> eventToReexecute_;

	/** Opt: Whether a particular read needs to be repaired during rescheduling */
	Event readToReschedule_ = Event::getInit();

	/* The read schedule() chose to retry, for the driver to unblock */
	std::optional<Event> readToUnblock_;

	/** Opt: Thread priorities */
	std::vector<Event> threadPrios_;

	/** Opt: Cached labels for optimized scheduling */
	ValuePrefixT seenPrefixes;
};

class ArbitraryScheduler : public Scheduler {
public:
	ArbitraryScheduler(const Config *config);

protected:
	using MyRNG = std::mt19937;
	using MyDist = std::uniform_int_distribution<MyRNG::result_type>;

	auto getRng() -> MyRNG & { return rng_; }

private:
	auto schedulePolicy(const ExecutionGraph &g, std::span<Action> runnable)
		-> std::optional<int> override;

	/** RNG for scheduling randomization */
	MyRNG rng_;
};

#define DEFINE_PURE_SCHEDULER_SUBCLASS(_policy, _base)                                             \
	class _policy##Scheduler : public _base {                                                  \
	public:                                                                                    \
		_policy##Scheduler(const Config *config) : _base(config) {}                        \
                                                                                                   \
	private:                                                                                   \
		virtual auto schedulePolicy(const ExecutionGraph &g, std::span<Action> runnable)   \
			-> std::optional<int> override;                                            \
	};

DEFINE_PURE_SCHEDULER_SUBCLASS(LTR, Scheduler);
DEFINE_PURE_SCHEDULER_SUBCLASS(WF, Scheduler);
DEFINE_PURE_SCHEDULER_SUBCLASS(WFR, ArbitraryScheduler)

#endif /* GENMC_SCHEDULER_HPP */
