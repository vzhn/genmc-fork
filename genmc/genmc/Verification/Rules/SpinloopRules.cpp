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


#include "genmc/Verification/Rules/SpinloopRules.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Execution/ModelInterfaces.hpp"
#include "genmc/Support/Error.hpp"

#include <algorithm>
#include <ranges>

namespace spin_rules {

auto startsFirstIteration(const ExecutionGraph &g, const SpinStartLabel &lab) -> bool
{
	const auto *lbLab = genmc::dyn_cast<LoopBeginLabel>(g.po_imm_pred(&lab));
	return lbLab && lbLab->getLoop() == lab.getLoop();
}

/* The FAI write of the FAI-ZNE loop LAB (a ZNE end or block) belongs to */
static auto getZNEFaiWrite(const ExecutionGraph &g, const EventLabel &lab) -> const FaiWriteLabel *
{
	auto preds = g.po_preds(&lab);
	auto wLabIt = std::ranges::find_if(
		preds, [](auto &pLab) { return genmc::isa<FaiWriteLabel>(&pLab); });
	VERIFY(wLabIt != std::ranges::end(preds));
	return genmc::dyn_cast<FaiWriteLabel>(&*wLabIt);
}

/* Whether LAB, an access to an FAI-ZNE loop's location, leaves the loop's
 * assumptions about the FAI write WLAB intact */
static auto keepsFaiAssumptions(const GraphModel &model, const FaiWriteLabel &wLab,
				const MemAccessLabel &lab) -> bool
{
	/* FAIs on the same variable are OK... */
	return genmc::isa<FaiReadLabel>(&lab) || genmc::isa<FaiWriteLabel>(&lab) ||
	       model.getHbView(&wLab).contains(lab.getPos());
}

auto faiZNEConstraintsSat(const ExecutionGraph &g, const GraphModel &model,
			  const FaiZNESpinEndLabel &lab) -> bool
{
	/* Check that there are no other side-effects since the previous iteration.
	 * We don't have to look for a BEGIN label since ZNE labels are always
	 * preceded by a spin-start */
	auto preds = g.po_preds(&lab);
	auto ssLabIt = std::ranges::find_if(
		preds, [](auto &pLab) { return genmc::isa<SpinStartLabel>(&pLab); });
	VERIFY(ssLabIt != preds.end());
	const auto *ssLab = &*ssLabIt;
	for (auto i = ssLab->getIndex() + 1; i < lab.getIndex(); ++i) {
		auto *oLab = g.getEventLabel(Event(ssLab->getThread(), i));
		if (genmc::isa<WriteLabel>(oLab) && !genmc::isa<FaiWriteLabel>(oLab))
			return false;
	}

	/* All stores in the RMW chain need to be read from at most 1 read,
	 * and there need to be no other stores that are not hb-before lab */
	const auto *wLab = getZNEFaiWrite(g, lab);
	return std::ranges::all_of(g.labels(), [&](auto &oLab) {
		auto *mLab = genmc::dyn_cast<MemAccessLabel>(&oLab);
		return !mLab || mLab->getAddr() != wLab->getAddr() ||
		       keepsFaiAssumptions(model, *wLab, *mLab);
	});
}

auto faiSpinloopsBrokenBy(const ExecutionGraph &g, const GraphModel &model,
			  const MemAccessLabel &lab) -> std::vector<const FaiZNEBlockLabel *>
{
	std::vector<const FaiZNEBlockLabel *> broken;
	for (auto i = 0; i < g.getNumThreads(); i++) {
		/* Is there any thread blocked on a potential spinloop? */
		auto *eLab = genmc::dyn_cast_if_present<FaiZNEBlockLabel>(g.getLastThreadLabel(i));
		if (!eLab)
			continue;

		/* Check whether this access affects the spinloop variable,
		 * and also breaks the assumptions */
		const auto *faiLab = getZNEFaiWrite(g, *eLab);
		if (faiLab->getAddr() == lab.getAddr() && !keepsFaiAssumptions(model, *faiLab, lab))
			broken.push_back(eLab);
	}
	return broken;
}

} // namespace spin_rules
