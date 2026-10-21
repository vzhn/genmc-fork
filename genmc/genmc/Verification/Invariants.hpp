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
 * -check-invariants: runtime checks of the bookkeeping that has produced the
 * most crashes and silent losses: graph restriction, error replay, the
 * allocation map rebuilt at the start of an execution, and the runtime
 * states forward revisits resume from. The option is off by default (and on
 * by default in ENABLE_GENMC_DEBUG builds); with it off nothing here runs.
 * A violation is an internal failure whose message names the invariant, the
 * events involved and the graph, printed as error reports print it. Each
 * check costs O(graph) and runs once per restriction, execution start or
 * resume, which the operations it checks cost themselves.
 *
 * I1 Restriction (checkGraph()). After every restriction of the graph
 *    (cutToStamp() for a forward revisit or a rerun, copyGraphUpTo() for a
 *    backward revisit or a graph handed to another worker, cutToView() when
 *    pruning, relocateThread() under LKMM) and on the graph a worker takes
 *    over, the graph is well-formed:
 *    - thread ids are dense: every thread has labels, thread 0 starts with
 *      the initializer and every other one with a begin whose thread info
 *      carries its id; every label is found at its position, the po lists
 *      agree with the thread vectors, the insertion order holds every label
 *      exactly once with strictly increasing stamps (dense after a cut);
 *    - every reference between labels points at a label of the graph: a
 *      read's rf source (which lists the read among its readers), a write's
 *      readers (which read from it), the coherence orders (no write twice),
 *      the initializer's readers, create <-> begin (the create's child is
 *      the begin's thread), join <-> finish; the cached prefix views and
 *      the calculated views of every label stay within the threads' sizes;
 *    - a begin whose create was cut is terminated, has no other label, and
 *      is not the last thread (such trailing threads are erased); a begin
 *      whose create is present is not terminated (the rule of PR #32, which
 *      kept begins attached to cut creates, erased the wrong thread and
 *      shifted thread ids);
 *    - holes (EMPTY labels) only under dependency-tracking models, never at
 *      index 0;
 *    - a cut keeps exactly the planned labels (RestrictionPlan, computed
 *      before the cut): nothing with a stamp above the cut survives, nothing
 *      at or below it is lost, except the begins of erased trailing threads,
 *      whose creates were cut.
 *    Validation: with beb1df1 reverted, tests/correct/infr/nested-create-*
 *    fail it at the first restriction that cuts a create.
 *
 * I2 Error replay (Scheduler.cpp, GenMCDriver.cpp). Whenever an error replay
 *    is pending, its target is in the schedule calculateReplaySchedule()
 *    returns (checked after the non-atomic accesses and the finished threads
 *    are erased, where the fix of PR #22 sits: the target used to be erased
 *    with the non-atomic accesses, and the error was lost or the next
 *    execution aborted); a new execution started in error replay has the
 *    buffered error and its target in the graph; initFromState() is entered
 *    with no pending replay (PR #19: a worker kept a stale target across the
 *    tasks it took); a kill event reached while driven by the graph is a
 *    kill label and is only re-executed by a replay that targets it or that
 *    keeps non-atomic events (PR #31).
 */

#ifndef GENMC_INVARIANTS_HPP
#define GENMC_INVARIANTS_HPP

#include "genmc/Execution/Event.hpp"
#include "genmc/Execution/Stamp.hpp"
#include "genmc/Verification/GenMCDriver.hpp"

#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

class ExecutionGraph;
class ExecutionState;
class VectorClock;

namespace invariants {

/** What the checks need to know about the situation they run in */
struct Context {
	/** The operation just performed (named in the report) */
	std::string_view where;
	/** Debugging information for printing the graph (may be null) */
	const GenMCDriver::GraphDbgInfo *dbgInfo = nullptr;
	/** Whether the graph may have holes (dependency-tracking models) */
	bool holesAllowed = false;
};

/** The labels a restriction must keep, computed before it (see I1) */
struct RestrictionPlan {
	/** Positions of the non-hole labels to keep, begins included */
	std::vector<Event> kept;
	/** Per thread, whether its create is among the kept labels */
	std::vector<bool> createKept;
	/** A description of the cut, for the report */
	std::string cut;
};

/** The plan of cutToStamp(ST) on G */
auto planCutToStamp(const ExecutionGraph &g, Stamp st) -> RestrictionPlan;

/** The plan of copyGraphUpTo(V) on G */
auto planCopyUpTo(const ExecutionGraph &g, const VectorClock &v) -> RestrictionPlan;

/** I1: G is well-formed, and (with PLAN) the restriction kept what it had to */
void checkGraph(const ExecutionGraph &g, const Context &ctx, const RestrictionPlan *plan = nullptr);

/** Reports a violation of INVARIANT (WHAT describes it) and aborts; G is printed if given */
[[noreturn]] void fail(std::string_view invariant, const std::string &what, const ExecutionGraph *g,
		       const Context &ctx,
		       std::source_location loc = std::source_location::current());

} // namespace invariants

#endif /* GENMC_INVARIANTS_HPP */
