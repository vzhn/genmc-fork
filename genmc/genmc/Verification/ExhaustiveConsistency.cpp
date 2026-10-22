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

#include "genmc/Verification/ExhaustiveConsistency.hpp"
#include "genmc/ADT/View.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Support/Error.hpp"

#include <algorithm>
#include <ranges>

auto ExhaustiveConsistency::recomputeViews(ExecutionGraph &g) -> bool
{
	/* A label's views come from those of the labels before it in hb, which
	 * need not come first in the insertion order: iterate until the hb
	 * views settle (they only grow after the first pass; an hb cycle makes
	 * them grow until the bound). A hole takes its po-predecessor's views,
	 * which the events kept after it inherit */
	const auto hb = model_.getHbViewIndex();
	auto same = [&g](const View &a, const View &b) {
		return std::ranges::all_of(std::views::iota(0, g.getNumThreads()),
					   [&](auto t) { return a.getMax(t) == b.getMax(t); });
	};
	auto settled = false;
	for (auto pass = 0U; !settled && pass <= g.getMaxStamp().get() + 1; pass++) {
		settled = true;
		for (auto &lab : g.labels()) {
			if (genmc::isa<InitLabel>(&lab))
				continue;
			const auto had = lab.views().size() > hb;
			const auto old = had ? lab.view(hb) : View();
			updateViews_(&lab);
			settled &= had && same(old, lab.view(hb));
		}
	}
	for (auto &lab : g.labels())
		if (!genmc::isa<InitLabel>(&lab))
			lab.setPrefixView(nullptr);
	return settled;
}

auto ExhaustiveConsistency::isGraphConsistent(const ExecutionGraph &g) const -> bool
{
	const auto &cc = model_;
	auto coIndex = [&g](const EventLabel *wLab) -> int {
		if (genmc::isa<InitLabel>(wLab))
			return -1;
		auto i = 0;
		for (const auto &oLab : g.co(genmc::cast<WriteLabel>(wLab)->getAddr())) {
			if (&oLab == wLab)
				return i;
			++i;
		}
		VERIFY(false);
		return -1;
	};

	/* Coherence: no cycle in hb;eco, as CoWR, CoRW, CoRR and CoWW */
	for (const auto &lab : g.labels()) {
		const auto *rLab = genmc::dyn_cast<ReadLabel>(&lab);
		if (!rLab || !rLab->getRf())
			continue;
		const auto &hbR = cc.getHbView(rLab);
		const auto rf = coIndex(rLab->getRf());
		auto i = 0;
		for (const auto &wLab : g.co(rLab->getAddr())) {
			if (i > rf && hbR.contains(wLab.getPos()))
				return false;
			if (i <= rf && cc.getHbView(&wLab).contains(rLab->getPos()))
				return false;
			++i;
		}
		for (const auto &oLab : g.labels()) {
			const auto *oRLab = genmc::dyn_cast<ReadLabel>(&oLab);
			if (oRLab && oRLab != rLab && oRLab->getRf() &&
			    oRLab->getAddr() == rLab->getAddr() && hbR.contains(oRLab->getPos()) &&
			    coIndex(oRLab->getRf()) > rf)
				return false;
		}
	}
	for (auto locIt = g.loc_begin(); locIt != g.loc_end(); ++locIt) {
		auto i = 0;
		for (const auto &wLab : g.co(locIt->first)) {
			auto j = 0;
			for (const auto &oLab : g.co(locIt->first)) {
				if (j++ >= i)
					break;
				if (cc.getHbView(&oLab).contains(wLab.getPos()))
					return false;
			}
			++i;
		}
	}

	/* Atomicity: an RMW's write follows the write its read reads from */
	for (const auto &lab : g.labels()) {
		const auto *wLab = genmc::dyn_cast<WriteLabel>(&lab);
		if (!wLab || !wLab->isRMW())
			continue;
		const auto *rLab = genmc::dyn_cast<ReadLabel>(g.po_imm_pred(wLab));
		const EventLabel *pLab = g.co_imm_pred(wLab);
		if (!rLab || rLab->getRf() != (pLab ? pLab : g.getInitLabel()))
			return false;
	}
	return cc.isConsistent(g);
}

auto ExhaustiveConsistency::isExecutionConsistent(const ExecutionGraph &g) -> bool
{
	auto copy = g.clone();
	model_.recomputeCacheCounters(*copy);
	const auto consistent = recomputeViews(*copy) && isGraphConsistent(*copy);
	model_.recomputeCacheCounters(g);
	return consistent;
}

/* The errors and warnings found at an event (checkForRaces()) are pairs
 * the views leave unordered: a race, unordered writes, an access to freed
 * memory. Under -exhaustive and IMM, the events a revisit kept after a
 * hole keep the views they had when they were kept (see
 * isExecutionConsistent()), and so do the events added after them: what
 * the event that fills the hole orders before them is missing (e.g., the
 * write a kept write's thread now reads with acquire before it). Such a
 * pair can be ordered once the views are computed anew, and the views
 * computed anew only grow: what they leave unordered is all that is.
 * (Whether the graph is consistent is checked before an error is
 * reported, see reportError().) */
void ExhaustiveConsistency::checkWithViewsAnew(
	const ExecutionGraph &g, const std::function<void(const ExecutionGraph &)> &check)
{
	auto copy = g.clone();
	model_.recomputeCacheCounters(*copy);
	recomputeViews(*copy);
	check(*copy);
	model_.recomputeCacheCounters(g);
}
