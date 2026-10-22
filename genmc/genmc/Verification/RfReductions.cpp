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


#include "genmc/Verification/RfReductions.hpp"
#include "genmc/Verification/BackwardRevisits.hpp"
#include "genmc/Models/SymmetryChecker.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/GraphUtils.hpp"
#include "genmc/Support/Error.hpp"
#include "genmc/Support/SExprVisitor.hpp"
#include "genmc/Support/ModuleVarID.hpp"

#include <algorithm>
#include <ranges>

void RfReductions::filter(const ReadLabel *lab, std::vector<EventLabel *> &stores) const
{
	/* Symmetry reduction */
	if (opts_.symmetryReduction)
		filterSymmetricStoresSR(lab, stores);

	/* BAM */
	if (!opts_.disableBAM)
		filterConflictingBarriers(lab, stores);

	/* Identical blind RMWs commute */
	if (opts_.blindRMWReduction)
		filterCommutingBlindRMWs(lab, stores);

	/* Keep values that do not lead to blocking (-exhaustive explores the
	 * blocked executions too: the annotations are not trusted) */
	if (!opts_.exhaustive)
		filterValuesFromAnnotSAVER(lab, stores);
}

void RfReductions::filterConflictingBarriers(const ReadLabel *lab, std::vector<EventLabel *> &stores) const
{
	if (opts_.disableBAM ||
	    (!genmc::isa<BIncFaiReadLabel>(lab) && !genmc::isa<BWaitReadLabel>(lab)))
		return;

	/* Helper lambdas */
	auto isReadByExclusiveRead = [&](auto *oLab) {
		if (auto *wLab = genmc::dyn_cast<WriteLabel>(oLab))
			return std::ranges::any_of(wLab->readers(),
						   [&](auto &rLab) { return rLab.isRMW(); });
		if (auto *iLab = genmc::dyn_cast<InitLabel>(oLab))
			return std::ranges::any_of(iLab->rfs(lab->getAddr()),
						   [&](auto &rLab) { return rLab.isRMW(); });
		UNREACHABLE();
	};
	auto findFaiReader = [](BIncFaiWriteLabel *wLab) {
		return std::ranges::find_if(wLab->readers(), [](auto &rLab) {
			return genmc::isa<BIncFaiReadLabel>(&rLab);
		});
	};
	auto findSameRoundMaximal = [&](BIncFaiWriteLabel *wLab) {
		auto &g = *wLab->getParent();
		while (!isLastInBarrierRound(wLab) &&
		       findFaiReader(wLab) != std::ranges::end(wLab->readers())) {
			wLab = genmc::dyn_cast<BIncFaiWriteLabel>(
				g.po_imm_succ(&*findFaiReader(wLab)));
		}
		return wLab;
	};

	/* barrier_wait()'s plain load should read maximally */
	if (genmc::isa<BWaitReadLabel>(lab)) {
		auto *wLab = genmc::dyn_cast<BIncFaiWriteLabel>(stores[0]);
		VERIFY(wLab && wLab->getPos().next() == lab->getPos());
		stores[0] = findSameRoundMaximal(wLab);
		stores.resize(1);
		return;
	}

	/* barrier_wait()'s FAI loads should not read from conflicting stores */
	std::erase_if(stores, [&](auto &sLab) { return isReadByExclusiveRead(sLab); });
}

void RfReductions::filterSymmetricStoresSR(const ReadLabel *rLab,
					  std::vector<EventLabel *> &stores) const
{
	const auto &g = g_;
	auto tid = g.getFirstThreadLabel(rLab->getThread())->getSymmPredTid();

	/* If there is no symmetric thread, exit */
	if (tid == -1)
		return;

	/* Check whether the po-prefixes of the two threads match */
	if (!symm_.sharePrefixSR(tid, rLab))
		return;

	/* Get the symmetric event and make sure it matches as well */
	const auto *lab = genmc::dyn_cast<ReadLabel>(g.getEventLabel(Event(tid, rLab->getIndex())));
	if (!lab || lab->getAddr() != rLab->getAddr() || lab->getSize() != lab->getSize())
		return;

	if (!lab->isRMW())
		return;

	/* Remove stores that will be explored symmetrically */
	std::erase_if(stores, [&](auto &sLab) { return lab->getRf()->getPos() == sLab->getPos(); });
}

/* The read part of WLAB if it is an RMW write, or null */
auto getRMWRead(const WriteLabel &wLab) -> const ReadLabel *
{
	const auto *rLab =
		genmc::dyn_cast_if_present<ReadLabel>(wLab.getParent()->po_imm_pred(&wLab));
	return rLab && rLab->isRMW() ? rLab : nullptr;
}

/*
 * Two blind relaxed FAIs that apply the same operation with the same operand
 * to the same location write the same sequence of values in either order, and
 * neither synchronizes with anything.  Executions that differ only in their
 * relative order are thus equivalent, and we keep the order in which they are
 * added: a new such FAI does not read from a store that an identical FAI
 * already reads from (which would place it before that FAI).  The co-maximal
 * store is always kept, so reads stay extensible.
 *
 * Reading co-maximally puts everything before the co-maximal store into the
 * new FAI's prefix, so later writes hb-after it can no longer revisit those
 * reads.  That loses no behavior only if (a) all writes to the location are
 * identical blind FAIs, so the only reads affected are theirs, whose revisits
 * are again swaps, and (b) no other read lies between the skipped store and the
 * co-maximal one in prefix order.  A different write to the location that
 * arrives later is reported by checkBlindRMWWellFormedness().
 */
void RfReductions::filterCommutingBlindRMWs(const ReadLabel *rLab,
					   std::vector<EventLabel *> &stores) const
{
	const auto &g = g_;
	const BlindRMWFilter blind(g, model_);
	const auto *fLab = blind.asCommuting(rLab);
	if (!fLab)
		return;

	auto addr = fLab->getAddr();

	/* (a): every write to the location so far is an identical blind FAI */
	if (!std::ranges::all_of(g.co(addr), [&](auto &wLab) {
		    const auto *pLab = getRMWRead(wLab);
		    return pLab && blind.areTwins(pLab, fLab);
	    }))
		return;

	const auto *coMax = g.co_max(addr);
	if (genmc::isa<InitLabel>(coMax))
		return;

	/* (b): between SLAB and the co-maximal store, the prefix holds only twins */
	const auto &maxView = prefixView(model_, coMax);
	const auto &ownView = prefixView(model_, fLab);
	auto onlyTwinsAfter = [&](const EventLabel *sLab) {
		const auto *sView = genmc::isa<InitLabel>(sLab) ? nullptr : &prefixView(model_, sLab);
		for (auto t = 0; t < g.getNumThreads(); t++) {
			auto from = std::max(ownView.getMax(t), sView ? sView->getMax(t) : -1);
			for (auto i = std::max(from + 1, 0); i <= maxView.getMax(t); i++) {
				const auto *lab = g.getEventLabel(Event(t, i));
				if (genmc::isa<ReadLabel>(lab) && !blind.areTwins(lab, fLab))
					return false;
			}
		}
		return true;
	};
	auto isReadByTwin = [&](const EventLabel *sLab) {
		auto isTwin = [&](auto &oLab) { return blind.areTwins(&oLab, fLab); };
		if (const auto *wLab = genmc::dyn_cast<WriteLabel>(sLab))
			return std::ranges::any_of(wLab->readers(), isTwin);
		return std::ranges::any_of(genmc::cast<InitLabel>(sLab)->rfs(addr), isTwin);
	};

	auto kept = stores;
	std::erase_if(kept, [&](auto *sLab) {
		return sLab != coMax && isReadByTwin(sLab) && onlyTwinsAfter(sLab);
	});
	if (!kept.empty())
		stores = std::move(kept);
}

void RfReductions::filterValuesFromAnnotSAVER(const ReadLabel *rLab,
					     std::vector<EventLabel *> &validStores) const
{
	/* Locks are treated as annotated CASes */
	if (!rLab->getAnnot())
		return;

	using Evaluator = SExprEvaluator<ModuleVarID>;

	const auto &g = g_;

	/* Ensure we keep the maximal store around even if Helper messed with it */
	VERIFY(!validStores.empty());
	auto *maximal = validStores.back();
	std::erase_if(validStores, [&](auto *wLab) {
		auto val = wLab->getAccessValue(rLab->getAccess());
		return wLab != maximal && wLab != g.co_max(rLab->getAddr()) &&
		       !Evaluator().evaluate(&*rLab->getAnnot()->expr, val);
	});
	VERIFY(!validStores.empty());
}
