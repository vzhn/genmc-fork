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


#ifndef GENMC_SPINLOOP_RULES_HPP
#define GENMC_SPINLOOP_RULES_HPP

/*
 * The decisions behind blocking a thread in a spinloop: each a pure function
 * of the graph and an event. A spinloop whose iteration has no side effects
 * (tracked by ExecutionState::onSpinStart()) is blocked at its next spin
 * start. An FAI-ZNE loop (an FAI followed by a test of its result) is blocked
 * once its FAI chain cannot be disturbed, and unblocked again by an access
 * that breaks that assumption. Blocking, and adding the labels, is the
 * driver's job.
 */

#include <vector>

class ExecutionGraph;
class FaiZNEBlockLabel;
class FaiZNESpinEndLabel;
class GraphModel;
class MemAccessLabel;
class SpinStartLabel;

namespace spin_rules {

/** Whether LAB starts its loop's first iteration (it directly follows the
 * loop's begin), so that there is no previous iteration to judge */
auto startsFirstIteration(const ExecutionGraph &g, const SpinStartLabel &lab) -> bool;

/** Whether the FAI-ZNE loop that ends at LAB may block: its iteration has no
 * writes besides its FAI, and every access to the FAI's location other than
 * an FAI is hb-before the FAI's write */
auto faiZNEConstraintsSat(const ExecutionGraph &g, const GraphModel &model,
			  const FaiZNESpinEndLabel &lab) -> bool;

/** The FAI-ZNE blocks (the last labels of their threads) whose assumptions
 * LAB breaks: LAB accesses the FAI's location, is not an FAI, and is not
 * hb-before the FAI's write. Their threads have to spin again */
auto faiSpinloopsBrokenBy(const ExecutionGraph &g, const GraphModel &model,
			  const MemAccessLabel &lab) -> std::vector<const FaiZNEBlockLabel *>;

} // namespace spin_rules

#endif /* GENMC_SPINLOOP_RULES_HPP */
