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

#include "genmc/Models/IMMFullChecker.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"

#include <algorithm>

auto IMMFullChecker::satisfiesExtraAxioms(const ExecutionGraph &g) -> bool
{
	/* The full ar differs from the explored one only by addr ; po+: without
	 * an address dependency followed by another event of its thread, the
	 * exploration's checks decided it */
	auto hasAddrPo = std::ranges::any_of(g.labels(), [&g](const EventLabel &lab) {
		return !std::ranges::empty(lab.addr()) && g.po_imm_succ(&lab) != nullptr;
	});
	return !hasAddrPo || arFull(g);
}
