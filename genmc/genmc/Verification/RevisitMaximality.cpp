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


#include "genmc/Verification/RevisitMaximality.hpp"
#include "genmc/ADT/DepView.hpp"
#include "genmc/ADT/VectorClock.hpp"
#include "genmc/Execution/Consistency/ModelInterfaces.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Support/Error.hpp"
#include "genmc/Verification/Revisit.hpp"

#include <algorithm>
#include <memory>
#include <ranges>

auto prefixView(const GraphModel &model, const EventLabel *lab) -> const VectorClock &
{
	if (!lab->hasPrefixView())
		lab->setPrefixView(model.calculatePrefixView(lab));
	return lab->getPrefixView();
}

auto revisitPrefixView(const GraphModel &model, bool scopesCtrlDeps, const EventLabel *lab)
	-> std::unique_ptr<VectorClock>
{
	if (!scopesCtrlDeps)
		return prefixView(model, lab).clone();
	return model.calculateRevisitPrefixView(lab);
}

static auto isReadInView(const WriteLabel *sLab, const DepView &v) -> bool
{
	return std::ranges::any_of(sLab->readers(),
				   [&v](auto &rLab) { return v.contains(rLab.getPos()); });
}

auto RevisitMaximality::isCoBeforeSavedPrefix(const BackwardRevisit &r, const EventLabel *lab) const -> bool
{
	const auto *mLab = genmc::dyn_cast<MemAccessLabel>(lab);
	if (!mLab)
		return false;

	const auto &g = g_;
	const auto &v = r.getViewNoRel();
	const auto *rLab = genmc::dyn_cast<ReadLabel>(mLab);
	auto *wLab = g.getWriteLabel(rLab ? rLab->getRf()->getPos() : mLab->getPos());

	auto succs = wLab ? g.co_succs(wLab) : g.co(mLab->getAddr());
	return std::ranges::any_of(succs, [&](auto &sLab) {
		/* Exclude the write that revisits from the prefix */
		return sLab.getPos() != r.getRev() && v->contains(sLab.getPos());
	});
}

/* Dependency-tracking models: R deletes LAB, or LAB is the read R revisits.
 * Whether LAB reads from (a read) or follows in coherence (a write) the
 * co-latest write it can reach among the writes R keeps, except the revisiting
 * one, and those added before LAB: TruSt's condition, that LAB made the
 * maximal choice among the writes present when it was added and that no kept
 * write could have revisited it (as isCoBeforeSavedPrefix() requires for the
 * other models), with the kept writes present from the start. If LAB chose
 * otherwise, the graph where it chose that write is the one R is explored
 * from.
 *
 * R keeps the revisiting write's prefix, which is not po-closed: it may keep
 * accesses of LAB's thread po-after LAB (LAB is then executed anew into a
 * hole), and a kept event was not always added after the deleted ones.
 * Coherence bounds LAB by the first po-later access of its thread to its
 * location present when LAB is added (kept, or added before LAB): LAB
 * precedes a write, and precedes or reads the write a read reads, as
 * coMaxReachable() bounds an access executed into a hole. The writes past
 * the bound are no alternatives. The added-maximally flag judges LAB against
 * the graph of the time it was added, which may lack a kept access that
 * bounds it (#121); and a kept write co-after LAB's is an alternative even
 * when its prefix reaches LAB's thread past LAB through a hole (#128) */
auto RevisitMaximality::isPlacedMaximallyInRevisit(const BackwardRevisit &r,
					     const MemAccessLabel *lab) const -> bool
{
	const auto &g = g_;
	const auto &v = r.getViewNoRel();

	/* A weak CAS that failed spuriously is the non-maximal twin of its success */
	if (const auto *cLab = genmc::dyn_cast<CasReadLabel>(lab); cLab && cLab->failsSpuriously())
		return false;

	/* An RMW's write part kept while its read is deleted follows the write
	 * that read gets when it is executed anew (updateKeptRMWWrite()): for a
	 * write LAB, it is placed when its read is added, not before */
	auto floats = [&](const EventLabel &eLab) {
		const auto *sLab = genmc::dyn_cast<WriteLabel>(&eLab);
		return genmc::isa<WriteLabel>(lab) && sLab && sLab->isRMW() &&
		       v->contains(eLab.getPos()) && !v->contains(eLab.getPos().prev());
	};
	auto isKept = [&](const EventLabel &eLab) {
		return v->contains(eLab.getPos()) && !floats(eLab);
	};
	auto addedAt = [&](const EventLabel &eLab) {
		return floats(eLab) ? g.getEventLabel(eLab.getPos().prev())->getStamp()
				    : eLab.getStamp();
	};
	auto competes = [&](const WriteLabel &sLab) {
		return sLab.getPos() != r.getRev() &&
		       (isKept(sLab) || addedAt(sLab) < lab->getStamp());
	};

	/* An RMW read's own write goes right after the read's write, wherever
	 * that is: it is no bound and no alternative, nor are the reads of it */
	const auto *rLab = genmc::dyn_cast<ReadLabel>(lab);
	const auto *ownLab = rLab && rLab->isRMW() ? g.getWriteLabel(lab->getPos().next())
						   : nullptr;

	/* The bound: LAB precedes BOUND, or may read it (BOUND a read's write) */
	const EventLabel *bound = nullptr;
	auto mayReadBound = false;
	for (auto j = lab->getIndex() + 1; j < g.getThreadSize(lab->getThread()); j++) {
		const auto *mLab = genmc::dyn_cast<MemAccessLabel>(
			g.getEventLabel(Event(lab->getThread(), j)));
		if (!mLab || mLab == ownLab || mLab->getAddr() != lab->getAddr())
			continue;
		/* An access deleted and added after LAB is re-added after it */
		if (!isKept(*mLab) && addedAt(*mLab) > lab->getStamp())
			continue;
		if (const auto *qLab = genmc::dyn_cast<ReadLabel>(mLab)) {
			if (!qLab->getRf() || qLab->getRf() == ownLab)
				continue;
			bound = qLab->getRf();
			mayReadBound = true;
		} else {
			bound = mLab;
		}
		break;
	}

	const auto *wLab = rLab ? rLab->getRf() : lab;
	VERIFY(wLab);

	/* A read reads a write added after it only through a revisit by that
	 * write: deleted, that write is no alternative, and the graph before
	 * its revisit builds the same graph (TruSt's revisitable flag) */
	const auto *rfLab = genmc::dyn_cast<WriteLabel>(wLab);
	if (rLab && rfLab && !competes(*rfLab))
		return false;
	if (bound && wLab == bound)
		return true;

	auto succs = rfLab ? g.co_succs(rfLab) : g.co(lab->getAddr());
	for (const auto &sLab : succs) {
		if (bound && &sLab == bound)
			return !(rLab && mayReadBound && competes(sLab));
		if (&sLab != ownLab && competes(sLab))
			return false;
	}
	return true;
}

/* Whether LAB, a write to the location of the revisiting write, follows in
 * coherence a write that the revisit keeps although that write was added after
 * LAB and is no RMW (it was placed non-maximally). Under LKMM, whose prefix keeps
 * a write po-before the revisiting one to the same location (moi), the revisit
 * deletes LAB and hides that placement; the same graph then follows from the
 * revisit of the kept write itself (tests/correct/lkmm/dup-revisit-nested-if).
 * Only when that other path exists: the read is no RMW and could read from the
 * kept write (which follows the read's write in coherence). Otherwise, the
 * graph where the kept write is placed maximally is rejected too (the read's
 * write precedes the kept one: isPlacedMaximallyInRevisit(), or
 * isCoBeforeSavedPrefix() for the other models), and this one is the only
 * path */
auto RevisitMaximality::isCoAfterLaterSavedWrite(const BackwardRevisit &r, const EventLabel *lab) const -> bool
{
	const auto *wLab = genmc::dyn_cast<WriteLabel>(lab);
	if (!wLab)
		return false;

	const auto &g = g_;
	const auto *rLab = g.getReadLabel(r.getPos());
	if (rLab->isRMW() || wLab->getAddr() != g.getWriteLabel(r.getRev())->getAddr())
		return false;

	/* The kept write could have been read instead of the read's write */
	const auto *rfLab = genmc::dyn_cast<WriteLabel>(rLab->getRf());
	auto readable = [&](auto &pLab) {
		return !rfLab || rfLab == &pLab ||
		       std::ranges::any_of(g.co_succs(rfLab),
					   [&](auto &sLab) { return &sLab == &pLab; });
	};
	const auto &v = r.getViewNoRel();
	return std::ranges::any_of(g.co_preds(wLab), [&](auto &pLab) {
		return pLab.getPos() != r.getRev() && !pLab.isRMW() && !pLab.wasAddedMax() &&
		       v->contains(pLab.getPos()) && pLab.getStamp() > wLab->getStamp() &&
		       readable(pLab);
	});
}

auto RevisitMaximality::coherenceSuccRemainInGraph(const BackwardRevisit &r) const -> bool
{
	const auto &g = g_;
	auto *wLab = g.getWriteLabel(r.getRev());
	if (wLab->isRMW())
		return true;

	auto succs = g.co_succs(wLab);
	if (std::ranges::empty(succs))
		return true;

	return r.getViewNoRel()->contains(std::ranges::begin(succs)->getPos());
}

static auto wasAddedMaximally(const EventLabel *lab) -> bool
{
	if (const auto *mLab = genmc::dyn_cast<MemAccessLabel>(lab))
		return mLab->wasAddedMax();
	if (const auto *oLab = genmc::dyn_cast<OptionalLabel>(lab))
		return !oLab->isExpanded();
	return true;
}

/* Whether LAB, not revisitable, is in the prefix of a write that R keeps and
 * that revisited a read still reading it (the read's stamp is below the
 * write's) */
auto RevisitMaximality::isKeptByKeptRevisit(const BackwardRevisit &r, const EventLabel *lab) const -> bool
{
	const auto &g = g_;
	const auto &v = r.getViewNoRel();
	return std::ranges::any_of(g.labels(), [&](auto &oLab) {
		const auto *uLab = genmc::dyn_cast<WriteLabel>(&oLab);
		if (!uLab || uLab->getPos() == r.getRev() || !v->contains(uLab->getPos()))
			return false;
		auto revisited = std::ranges::any_of(uLab->readers(), [&](auto &qLab) {
			return qLab.getStamp() < uLab->getStamp();
		});
		return revisited && getRevisitPrefixView(uLab)->contains(lab->getPos());
	});
}

auto RevisitMaximality::isMaximalExtension(const BackwardRevisit &r) const -> bool
{
	/* Only revisit when the write's direct successor (if any) remains in the graph;
	 * revisits should not only differ on the write's placement */
	if (!coherenceSuccRemainInGraph(r))
		return false;

	const auto &g = g_;
	const auto &v = r.getViewNoRel();

	const auto *depV =
		opts_.depTracking
			? genmc::dyn_cast<DepView>(&getPrefixView(g.getEventLabel(r.getRev())))
			: nullptr;

	/* Dependency-tracking models judge the accesses the revisit deletes
	 * against the writes they can reach (isPlacedMaximallyInRevisit()), not
	 * by their flags. A deleted event that an earlier revisit kept (not
	 * revisitable) still rejects the revisit, since the graph before that
	 * revisit leads to the same one, unless this revisit keeps the earlier
	 * revisiting write: that write is then an alternative for the read it
	 * revisited, and the other graph is the one rejected */
	for (const auto &lab : g.labels()) {
		/* Exclude events unaffected by the revisit */
		if (lab.getPos() != r.getPos() && v->contains(lab.getPos()))
			continue;

		if (depV) {
			if (const auto *sLab = genmc::dyn_cast<WriteLabel>(&lab);
			    sLab && isReadInView(sLab, *depV))
				continue;
			if (!lab.isRevisitable() && !isKeptByKeptRevisit(r, &lab))
				return false;
			/* (Under BAM, a barrier wait reads its round's last increment,
			 * revisited in place: no choice of its own to judge) */
			const auto *mLab = genmc::dyn_cast<MemAccessLabel>(&lab);
			if (mLab && (opts_.disableBAM || !genmc::isa<BWaitReadLabel>(mLab))) {
				if (!isPlacedMaximallyInRevisit(r, mLab))
					return false;
			} else if (!wasAddedMaximally(&lab)) {
				return false;
			}
			if (isCoAfterLaterSavedWrite(r, &lab))
				return false;
			continue;
		}

		if (!lab.isRevisitable())
			return false;
		if (!wasAddedMaximally(&lab))
			return false;
		if (isCoBeforeSavedPrefix(r, &lab))
			return false;
		if (isCoAfterLaterSavedWrite(r, &lab))
			return false;
	}
	return true;
}

/* The revisit keeps the events added before the read and the write's prefix.
 * A kept read may read from a write the revisit deletes: one added after the
 * read that revisited it earlier (the read's thread may have kept events in
 * between, through that write's prefix). TruSt rejects such a revisit by the
 * revisitable flag it set on that write's prefix; -exhaustive ignores the
 * flags, so it checks the graph: every kept read keeps its write */
auto RevisitMaximality::isRevisitViewClosed(const BackwardRevisit &r) const -> bool
{
	const auto &g = g_;
	const auto &v = r.getViewNoRel();
	return std::ranges::all_of(g.labels(), [&](auto &lab) {
		const auto *rLab = genmc::dyn_cast<ReadLabel>(&lab);
		if (!rLab || rLab->getPos() == r.getPos() || !v->contains(rLab->getPos()) ||
		    !rLab->getRf())
			return true;
		return v->contains(rLab->getRf()->getPos());
	});
}

/* The revisiting RMW's read keeps its write W; another RMW reading from W in
 * the revisit's view would make both RMWs read W, a graph no extension
 * repairs (and backwardRevisit() requires its absence) */
auto RevisitMaximality::keepsConflictingRMW(const BackwardRevisit &r) const -> bool
{
	const auto &g = g_;
	const auto *sLab = g.getWriteLabel(r.getRev());
	if (!sLab->isRMW())
		return false;
	const auto *pLab = genmc::dyn_cast<ReadLabel>(g.po_imm_pred(sLab));
	VERIFY(pLab && pLab->getRf());
	/* (The revisited read is in its own view, but it will read from SLAB) */
	const auto &v = r.getViewNoRel();
	auto conflicts = [&](const ReadLabel &rLab) {
		return &rLab != pLab && rLab.getPos() != r.getPos() && rLab.isRMW() &&
		       v->contains(rLab.getPos());
	};
	if (const auto *wLab = genmc::dyn_cast<WriteLabel>(pLab->getRf()))
		return std::ranges::any_of(wLab->readers(), conflicts);
	return std::ranges::any_of(g.getInitLabel()->rfs(pLab->getAddr()), conflicts);
}

