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


#ifndef GENMC_KEPT_EVENT_ALIGNMENT_HPP
#define GENMC_KEPT_EVENT_ALIGNMENT_HPP

/*
 * Where a thread's new path meets the events a revisit kept (under LKMM, and
 * under IMM with -exhaustive): kept events may sit where the path does not
 * reach them, e.g., after a control dependency ends at the join of its
 * branch. As a pure function of the graph: what to do when an instruction
 * instance runs at a position of its thread. Moving the events, dropping or
 * repeating the run, is the driver's job (ExplorationCore::alignKeptEvents()).
 */

#include "genmc/Support/Event.hpp"

#include <cstdint>

class EventKey;
class ExecutionGraph;

namespace kept_events {

struct Alignment {
	enum class Kind : std::uint8_t {
		/** The label at the position is the instance's own: replay it */
		Own,
		/** The label there has no key (an internal error) */
		MissingKey,
		/** Another instance's event, and this instance's is kept further
		 * on: the events in between would be skipped, so there is no such
		 * run. (Its event is moved out of the way before the run is dropped) */
		ShiftAndDrop,
		/** Another instance's event: open a gap for this one */
		OpenGap,
		/** A hole with no kept events after it: fill it */
		FillHole,
		/** A hole, then other kept events before this instance's: no such
		 * run, as for ShiftAndDrop */
		Drop,
		/** A hole before kept events none of which is this instance's: fill
		 * it (the graph has to be checked whole at the end of the run) */
		FillBeforeKept,
		/** Holes, then this instance's event (at FROM): move it here */
		MoveHere,
	};

	Kind kind;
	int from = 0; /* MoveHere: where the instance's event is */

	auto operator==(const Alignment &) const -> bool = default;
};

/** What to do when the instruction instance KEY runs its SUB-th event at CURR */
auto align(const ExecutionGraph &g, Event curr, const EventKey &key, std::uint32_t sub)
	-> Alignment;

} // namespace kept_events

#endif /* GENMC_KEPT_EVENT_ALIGNMENT_HPP */
