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

#ifndef GENMC_ERROR_RULES_HPP
#define GENMC_ERROR_RULES_HPP

/*
 * The memory-safety rules: each a pure function of the graph's state and an
 * event, returning the violation it finds. Reporting it (halting, replaying
 * the error for its trace) is the driver's job.
 */

#include "genmc/Support/ActionEnums.hpp"
#include "genmc/Support/Event.hpp"
#include "genmc/Support/MemAccess.hpp"
#include "genmc/Support/SAddr.hpp"
#include "genmc/Support/VerificationError.hpp"

#include <optional>
#include <string>

class BIncFaiWriteLabel;
class EventLabel;
class ExecutionGraph;
class ExecutionState;
class GraphModel;
class MemAccessLabel;
class ReadLabel;
class ThreadFinishLabel;
class WriteLabel;

namespace error_rules {

/** Why freeing LOC (with storage duration SDUR) is invalid, if it is */
auto invalidFreeReason(const ExecutionState &state, SAddr loc, StorageDuration sdur)
	-> std::optional<std::string>;

/** An access to unallocated memory, or (with STATICCHECKS) a static access
 * outside every registered global */
auto invalidAccess(const ExecutionState &state, bool staticChecks, Event pos,
		   const AAccess &access) -> std::optional<ErrorDetails>;

/** A dynamic read of memory not initialized before it (in hb), or reading
 * a write of another size */
auto uninitializedRead(const GraphModel &model, const ReadLabel *rLab)
	-> std::optional<ErrorDetails>;

/** An unlock of a mutex the thread does not hold */
auto invalidUnlock(const WriteLabel *wLab) -> std::optional<ErrorDetails>;

/** Whether a warning WCODE at POS is treated as an error, and its message.
 * Unordered writes are an error when symmetry reduction (with symmetric
 * threads) or in-place revisiting (with annotated reads of the location)
 * relies on their absence: verification may be repeated without them */
struct WarningUpgrade {
	bool upgrade = false;
	bool iprError = false;		    /**< caused by IPR (else by SR) */
	bool inferredSymmetryError = false; /**< SR with symmetries GenMC inferred */
	std::string msg;
};
auto wwRaceUpgrade(const ExecutionGraph &g, Event pos, VerificationError wcode,
		   bool symmetryReduction, bool ipr) -> WarningUpgrade;

/** An atomic access that overlaps a differently-sized atomic one */
auto mixedSizeAccess(const MemAccessLabel *lab) -> std::optional<ErrorDetails>;

/** If LAB accesses another thread's copy of a thread-local (see SAddr),
 * that thread, and 0 otherwise */
auto threadLocalOwner(const EventLabel *lab) -> int;

/** LAB, an access to a thread-local of the thread ELAB ends, does not happen
 * before the end */
auto threadLocalAfterEnd(const GraphModel &model, const EventLabel *lab,
			 const ThreadFinishLabel *eLab) -> std::optional<ErrorDetails>;

/** An access to a thread-local of the thread ELAB ends that does not happen
 * before the end (the main thread's copies last as long as the program) */
auto endedThreadLocal(const GraphModel &model, const ExecutionGraph &g,
		      const ThreadFinishLabel *eLab) -> std::optional<ErrorDetails>;

/** A store to a location with a final write that is not hb-before that
 * write, or a second final write (-final-write) */
auto finalWriteViolation(const GraphModel &model, const ExecutionGraph &g, const WriteLabel *wLab)
	-> std::optional<ErrorDetails>;

/** In-place revisiting: an annotated read RLAB of a location with unordered
 * writes, which IPR relies on not being there */
auto iprUnorderedWrites(const ExecutionGraph &g, const ReadLabel *rLab)
	-> std::optional<ErrorDetails>;

/** Blind-RMW reduction: RLAB observes the order of two identical blind RMWs */
auto blindRMWOrderObserved(const GraphModel &model, const ExecutionGraph &g, const ReadLabel *rLab)
	-> std::optional<ErrorDetails>;

/** Blind-RMW reduction: SLAB, a write to a location of blind RMWs that is
 * not one of them, is not hb-after them */
auto blindRMWReorderedWith(const GraphModel &model, const ExecutionGraph &g, const WriteLabel *sLab)
	-> std::optional<ErrorDetails>;

/** BAM: the barrier increment SLAB does not follow the last completed
 * round of its barrier in hb;po */
auto barrierNotWellFormed(const GraphModel &model, const ExecutionGraph &g,
			  const BIncFaiWriteLabel *sLab) -> std::optional<ErrorDetails>;

/** A data race of an hb-based model (C11, IMM) that involves LAB: an access
 * of another thread that overlaps it, with a write and a non-atomic access
 * between the two, ordered with it by hb in neither direction. With
 * SKIPAFTERHOLES, accesses that come after a hole in their thread are not
 * considered (see ExplorationCore::checkForRaces()). The racy label is the
 * other access */
auto hbRaceWith(const GraphModel &model, const ExecutionGraph &g, const EventLabel *lab,
		bool skipAfterHoles) -> std::optional<ErrorDetails>;

/** A data race of an hb-based model anywhere in G, reported at the access
 * added later (by stamp) */
auto hbRace(const GraphModel &model, const ExecutionGraph &g) -> std::optional<ErrorDetails>;

} // namespace error_rules

#endif /* GENMC_ERROR_RULES_HPP */
