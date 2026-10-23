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


#ifndef GENMC_HELPING_CAS_RULES_HPP
#define GENMC_HELPING_CAS_RULES_HPP

/*
 * The helping-CAS optimization: a helping CAS that would only repeat a
 * helped CAS (same location, values and ordering) is not explored on its
 * own; its thread blocks until a helped CAS appears. Sound only if the
 * annotations hold, which these rules check, each a pure function of the
 * graph. A misannotation is a user error, which the driver raises.
 */

#include <optional>
#include <string_view>
#include <vector>

class ExecutionGraph;
class GraphModel;
class HelpedCasReadLabel;
class HelpingCasLabel;

namespace helping_cas {

/** The helped CASes (that succeeded) that HELPING would repeat */
auto helpedCases(const ExecutionGraph &g, const HelpingCasLabel &helping)
	-> std::vector<const HelpedCasReadLabel *>;

/** Whether HELPING sees each thread's last store before HELPED (in hb) */
auto seesStoresBeforeHelped(const ExecutionGraph &g, const GraphModel &model,
			    const HelpedCasReadLabel &helped, const HelpingCasLabel &helping)
	-> bool;

/** At the end of an execution: a thread still waits for a helped CAS, or a
 * store a helping CAS would succeed on is read by no helped CAS. Returns
 * what is wrong */
auto annotationError(const ExecutionGraph &g) -> std::optional<std::string_view>;

} // namespace helping_cas

#endif /* GENMC_HELPING_CAS_RULES_HPP */
