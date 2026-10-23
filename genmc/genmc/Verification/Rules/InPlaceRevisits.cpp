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


#include "genmc/Verification/Rules/InPlaceRevisits.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Execution/GraphUtils.hpp"
#include "genmc/Support/Error.hpp"

namespace in_place {

auto barrierWaits(const BIncFaiWriteLabel &sLab) -> std::vector<Event>
{
	/* Walk the round back through the increments' rfs. (The reads the
	 * revisit was computed for are disregarded, as they might not contain
	 * some valid revisits, e.g., discarded due to maximality-related
	 * optimizations) */
	const auto &g = *sLab.getParent();
	const auto *wLab = genmc::dyn_cast<ReadLabel>(g.po_imm_pred(&sLab))->getRf();
	VERIFY(wLab);
	std::vector<Event> waits;

	while (genmc::isa<BIncFaiWriteLabel>(wLab) &&
	       !isLastInBarrierRound(genmc::dyn_cast<BIncFaiWriteLabel>(wLab))) {
		const auto *nLab = g.po_imm_succ(wLab);
		if (nLab) {
			VERIFY(genmc::isa<BWaitReadLabel>(nLab));
			waits.push_back(nLab->getPos());
		}
		wLab = genmc::dyn_cast<ReadLabel>(g.po_imm_pred(wLab))->getRf();
	}
	return waits;
}

auto blocksOnAnnotation(const ReadLabel &rLab) -> bool
{
	/* Treatment of blocked CASes is different */
	return !genmc::isa<CasReadLabel>(&rLab) && rLab.getAnnot() &&
	       !rLab.valueMakesAssumeSucceed(rLab.getAccessValue(rLab.getAccess()));
}

} // namespace in_place
