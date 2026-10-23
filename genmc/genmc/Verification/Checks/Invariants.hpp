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
 *
 * I3 Allocation map (checkAllocations()). After reconstructState() and after
 *    every allocation: the registered blocks are exactly the MALLOC labels
 *    of the graph (a null address is a failed allocation), each at its
 *    label's address and size, with the label's thread in the address; a
 *    block is marked freed exactly when the graph has a free of it; each
 *    thread's allocator pool lies past every block of the thread, so that a
 *    later allocation cannot overlap a block the graph holds (PR #66: the
 *    map was rebuilt by allocating anew in po order, and two stack variables
 *    overlapped after a stamp cut under IMM).
 *
 * I4 Resume points (fingerprint(), checkResume()). When the runtime's state
 *    is saved for the forward revisits of an event, the graph is summarised
 *    (GraphFingerprint: per thread the last index and a hash of the labels'
 *    kinds, positions, addresses, sizes, values, rf sources and thread
 *    links; the coherence orders; the revisited event's own rf and co are
 *    left out since the revisit changes them). When a revisit resumes from
 *    the state, the graph restricted for it must have the same fingerprint,
 *    and the state's per-thread positions must not sit past a hole of the
 *    graph (a thread cannot have passed an event the graph lacks).
 *    (Issue #111: executions lost under IMM when resuming from older states.)
 */

#ifndef GENMC_INVARIANTS_HPP
#define GENMC_INVARIANTS_HPP

#include "genmc/Support/Event.hpp"
#include "genmc/Execution/Stamp.hpp"
#include "genmc/Verification/Report/DebugInfo.hpp"
#include "genmc/Verification/Checks/GraphFingerprint.hpp"

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
	const GraphDbgInfo *dbgInfo = nullptr;
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

/** I1 before a copy: the view V a backward revisit copies names only labels
 * of G. A view built from a stale prefix view reaches beyond a thread, and
 * the copy dereferences the missing labels before any check of its result */
void checkCopyView(const ExecutionGraph &g, const VectorClock &v, const Context &ctx);

/** I3: the allocation map of STATE agrees with the lifecycle labels of G */
void checkAllocations(const ExecutionGraph &g, const ExecutionState &state, const Context &ctx);

/** I4: summarises G for the revisits of SAVED */
auto fingerprint(const ExecutionGraph &g, Event saved) -> GraphFingerprint;

/** I4: how G (restricted for a revisit of SAVED's event) differs from the
 * fingerprint taken when the state was saved, if it does; POSITIONS are the
 * state's per-thread positions (empty if unknown) */
auto resumeMismatch(const ExecutionGraph &g, const GraphFingerprint &saved,
		    const std::vector<Event> &positions) -> std::optional<std::string>;

/** I4: fails on a mismatch (see resumeMismatch()) */
void checkResume(const ExecutionGraph &g, const GraphFingerprint &saved,
		 const std::vector<Event> &positions, const Context &ctx);

/** I4: SCHEDULE (the scheduler's replay schedule, latest first) drives every
 * thread of G through the holes before its kept events */
void checkResumeSchedule(const ExecutionGraph &g, const std::vector<Event> &schedule,
			 const Context &ctx);

/** I5: G, a complete execution the exploration counts, satisfies the memory
 * model's axioms as a whole (CONSISTENT: the model's whole-graph check).
 * The exploration checks each event as it adds it, and relies on those
 * checks implying the whole-graph one */
void checkConsistency(const ExecutionGraph &g, bool consistent, const Context &ctx);

/** Reports a violation of INVARIANT (WHAT describes it) and aborts; G is printed if given */
[[noreturn]] void fail(std::string_view invariant, const std::string &what, const ExecutionGraph *g,
		       const Context &ctx,
		       std::source_location loc = std::source_location::current());

} // namespace invariants

#endif /* GENMC_INVARIANTS_HPP */
