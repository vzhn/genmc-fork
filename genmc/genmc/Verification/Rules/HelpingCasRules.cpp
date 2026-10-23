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


#include "genmc/Verification/Rules/HelpingCasRules.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Execution/ModelInterfaces.hpp"

#include <algorithm>
#include <ranges>

namespace helping_cas {

auto helpedCases(const ExecutionGraph &g, const HelpingCasLabel &helping)
	-> std::vector<const HelpedCasReadLabel *>
{
	std::vector<const HelpedCasReadLabel *> helped;
	for (const auto &lab : g.labels()) {
		const auto *rLab = genmc::dyn_cast<HelpedCasReadLabel>(&lab);
		if (rLab && rLab->isRMW() && rLab->getAddr() == helping.getAddr() &&
		    rLab->getSize() == helping.getSize() &&
		    rLab->getOrdering() == helping.getOrdering() &&
		    rLab->getExpected() == helping.getExpected() &&
		    rLab->getSwapVal() == helping.getSwapVal())
			helped.push_back(rLab);
	}
	return helped;
}

auto seesStoresBeforeHelped(const ExecutionGraph &g, const GraphModel &model,
			    const HelpedCasReadLabel &helped, const HelpingCasLabel &helping)
	-> bool
{
	const auto &hb = model.getHbView(&helped);
	const auto &view = model.getHbView(&helping);

	for (auto i = 0; i < hb.size(); i++) {
		auto j = hb.getMax(i);
		while (!genmc::isa<WriteLabel>(g.getEventLabel(Event(i, j))) && j > 0)
			--j;
		if (j > 0 && !view.contains(Event(i, j)))
			return false;
	}
	return true;
}

auto annotationError(const ExecutionGraph &g) -> std::optional<std::string_view>
{
	/* If we were waiting for a helped CAS that did not appear, complain */
	for (auto i = 0; i < g.getNumThreads(); i++) {
		if (genmc::isa<HelpedCASBlockLabel>(g.getLastThreadLabel(i)))
			return "Does helped CAS always execute?";
	}

	/* Next, we need to check whether there are any extraneous
	 * stores, not visible to the helped/helping CAS */
	for (const auto &lab : g.labels()) {
		const auto *hLab = genmc::dyn_cast<HelpingCasLabel>(&lab);
		if (!hLab)
			continue;

		/* Check that all stores that would make this helping
		 * CAS succeed are read by a helped CAS.
		 * We don't need to check the swap value of the helped CAS */
		if (std::ranges::any_of(g.co(hLab->getAddr()), [&](auto &sLab) {
			    return hLab->getExpected() == sLab.getVal() &&
				   std::ranges::none_of(sLab.readers(), [&](auto &rLab) {
					   return genmc::isa<HelpedCasReadLabel>(&rLab);
				   });
		    }))
			return "Unordered store to helping CAS location!";

		/* Special case for the initializer (as above) */
		if (hLab->getAddr().isStatic() &&
		    hLab->getExpected() == g.getInitVal(hLab->getAccess())) {
			auto rsView = g.labels() | std::views::filter([hLab](auto &oLab) {
					      auto *rLab = genmc::dyn_cast<ReadLabel>(&oLab);
					      return rLab && rLab->getAddr() == hLab->getAddr();
				      });
			if (std::ranges::none_of(rsView, [&](auto &oLab) {
				    return genmc::isa<HelpedCasReadLabel>(&oLab);
			    }))
				return "Unordered store to helping CAS location!";
		}
	}
	return std::nullopt;
}

} // namespace helping_cas
