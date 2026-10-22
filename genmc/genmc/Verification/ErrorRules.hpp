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

class ExecutionGraph;
class ExecutionState;
class GraphModel;
class MemAccessLabel;
class ReadLabel;
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

} // namespace error_rules

#endif /* GENMC_ERROR_RULES_HPP */
