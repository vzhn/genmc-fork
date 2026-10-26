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


#include "genmc/Verification/Rules/BackwardRevisits.hpp"
#include "genmc/ADT/DepView.hpp"
#include "genmc/ADT/VectorClock.hpp"
#include "genmc/Execution/ModelInterfaces.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/DepExecutionGraph.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Execution/GraphUtils.hpp"
#include "genmc/Support/Error.hpp"
#include "genmc/Verification/State/Revisit.hpp"

#include <algorithm>
#include <memory>
#include <ranges>
#include <utility>

/* The prefix a revisit by LAB keeps, cached in LAB: the model's (its kat's
 * ppo and rf), or, with DEPENDENCIESONLY (-exhaustive under -lkmm), what LAB
 * depends on (see dependencyPrefixView()); the model's rules that look at
 * a write's prefix (which later reads of a thread a revisit keeps, say) then
 * see that one */
auto prefixView(const GraphModel &model, const EventLabel *lab, bool dependenciesOnly)
	-> const VectorClock &
{
	if (!lab->hasPrefixView())
		lab->setPrefixView(dependenciesOnly ? dependencyPrefixView(*lab->getParent(), lab)
						    : model.calculatePrefixView(lab));
	return lab->getPrefixView();
}

auto revisitPrefixView(const GraphModel &model, bool scopesCtrlDeps, const EventLabel *lab)
	-> std::unique_ptr<VectorClock>
{
	if (!scopesCtrlDeps)
		return prefixView(model, lab).clone();
	return model.calculateRevisitPrefixView(lab);
}

auto dependencyPrefixView(const ExecutionGraph &g, const EventLabel *lab)
	-> std::unique_ptr<VectorClock>
{
	auto v = std::make_unique<DepView>();
	std::vector<char> seen(g.getMaxStamp().get() + 1, 0);
	std::vector<const EventLabel *> work;
	auto add = [&](Event e) {
		if (e.isInitializer())
			return;
		const auto *eLab = g.getEventLabel(e);
		if (std::exchange(seen[eLab->getStamp().get()], 1))
			return;
		v->updateIdx(e);
		work.push_back(eLab);
	};
	auto addBefore = [&](const EventLabel *eLab) {
		for (auto i = 0; i < eLab->getIndex(); i++)
			add(Event(eLab->getThread(), i));
	};
	add(lab->getPos());
	while (!work.empty()) {
		const auto *eLab = work.back();
		work.pop_back();
		const auto &deps = eLab->getDeps();
		/* (addrPo, IMM's addr ; po, orders; pctrl is the shadow rule) */
		for (const auto *d : {&deps.addr, &deps.data, &deps.ctrl, &deps.cas})
			for (const auto &e : *d)
				add(e);
		if (const auto *rLab = genmc::dyn_cast<ReadLabel>(eLab); rLab && rLab->getRf())
			add(rLab->getRf()->getPos());
		if (const auto *wLab = genmc::dyn_cast<WriteLabel>(eLab); wLab && wLab->isRMW())
			add(eLab->getPos().prev());
		/* thread-po: a thread's start before its events, all of a thread
		 * before its end and before a create, and the end of a thread
		 * before its join; tc: a start's create */
		add(Event(eLab->getThread(), 0));
		if (const auto *bLab = genmc::dyn_cast<ThreadStartLabel>(eLab); bLab && bLab->getCreate())
			add(bLab->getCreate()->getPos());
		if (genmc::isa<ThreadCreateLabel>(eLab) || genmc::isa<ThreadFinishLabel>(eLab))
			addBefore(eLab);
		if (const auto *jLab = genmc::dyn_cast<ThreadJoinLabel>(eLab))
			add(g.getLastThreadLabel(jLab->getChildId())->getPos());
		for (auto i = 1; i < eLab->getIndex(); i++)
			if (genmc::isa<ThreadJoinLabel>(g.getEventLabel(Event(eLab->getThread(), i))))
				add(Event(eLab->getThread(), i));
	}
	return v;
}

auto dependentsOf(const ExecutionGraph &g, const ReadLabel *rLab, DependentSeeds seeds)
	-> std::vector<char>
{
	std::vector<char> in(g.getMaxStamp().get() + 1, 0);
	auto dependsOnOne = [&](const EventLabel *lab) {
		const auto &deps = lab->getDeps();
		for (const auto *d : {&deps.addr, &deps.data, &deps.ctrl, &deps.cas})
			for (const auto &e : *d)
				if (!e.isInitializer() && in[g.getEventLabel(e)->getStamp().get()])
					return true;
		if (const auto *oLab = genmc::dyn_cast<ReadLabel>(lab); oLab && oLab->getRf() &&
		    !oLab->getRf()->getPos().isInitializer() &&
		    in[oLab->getRf()->getStamp().get()])
			return true;
		if (const auto *wLab = genmc::dyn_cast<WriteLabel>(lab); wLab && wLab->isRMW() &&
		    in[g.po_imm_pred(lab)->getStamp().get()])
			return true;
		/* (A thread whose create was cut keeps only its begin, which
		 * records the create's old position: no create to depend on) */
		const auto *bLab = genmc::cast<ThreadStartLabel>(g.getFirstThreadLabel(lab->getThread()));
		if (const auto *tcLab = bLab->getCreate(); tcLab && in[tcLab->getStamp().get()])
			return true;
		return false;
	};
	if (seeds != DependentSeeds::Read) {
		for (const auto &lab : g.labels())
			if (lab.getDeps().sctrl.contains(rLab->getPos()))
				in[lab.getStamp().get()] = 1;
	}
	/* (RLAB itself: the revisit remakes it) */
	if (seeds != DependentSeeds::Branches)
		in[rLab->getStamp().get()] = 1;
	/* (A kept event may depend on one with a newer stamp: iterate) */
	for (auto changed = true; changed;) {
		changed = false;
		for (const auto &lab : g.labels()) {
			if (in[lab.getStamp().get()] || !dependsOnOne(&lab))
				continue;
			in[lab.getStamp().get()] = 1;
			changed = true;
		}
	}
	return in;
}

auto keptPrefixView(const ExecutionGraph &g, const GraphModel &model, const ReadLabel *rLab,
		    const WriteLabel *sLab) -> std::unique_ptr<VectorClock>
{
	if (dependentsOf(g, rLab)[sLab->getStamp().get()])
		return nullptr;
	/* RLAB in the model's prefix of SLAB through the write of its RMW (which
	 * another thread's RMW reads, say): the prefix is computed without
	 * every dependent of RLAB (see the header) */
	const auto fullPrefix = model.calculatePrefixView(sLab);
	const auto *wLab = genmc::dyn_cast_if_present<WriteLabel>(
		rLab->isRMW() ? g.po_imm_succ(rLab) : nullptr);
	const auto throughWrite = wLab && wLab->isRMW() && fullPrefix->contains(wLab->getPos());
	auto masked = dependentsOf(
		g, rLab, throughWrite ? DependentSeeds::Scoped : DependentSeeds::Branches);
	masked[rLab->getStamp().get()] = 0;

	/* With nothing to mask, the model's prefix itself (which has none of
	 * RLAB's dependents if it does not have RLAB: it has the read of an
	 * RMW's write, and the reads of the writes it has) */
	if (std::ranges::none_of(g.labels(),
				 [&](const auto &lab) { return masked[lab.getStamp().get()]; })) {
		auto prefix = model.calculateRevisitPrefixView(sLab);
		return prefix->contains(rLab->getPos()) ? nullptr : std::move(prefix);
	}
	/* The prefix is computed in a copy (positions name the same events in
	 * both) where each event of a branch RLAB controls and what depends on
	 * one (with THROUGHWRITE, every dependent of RLAB) is an inert event: a
	 * plain read of a private location, with no dependencies, reading the
	 * initializer. The model's prefix passes through it as po passes
	 * through anything, but it ends no path of the prefix (a hole would: a
	 * hole is not plain, so a release's ordering would reach past the plain
	 * read it stops at). RLAB stays as it is: what orders it before SLAB by
	 * what it is (an acquire read before all after it) orders it whatever
	 * it reads */
	auto all = g.getViewFromStamp(g.getMaxStamp());
	auto &dall = *genmc::cast<DepView>(&*all);
	for (const auto &lab : g.labels())
		if (masked[lab.getStamp().get()])
			dall.addHole(lab.getPos());
	auto copy = g.getCopyUpTo(*all);
	for (const auto &lab : g.labels()) {
		if (!masked[lab.getStamp().get()])
			continue;
		/* (A dependent at the end of its thread is not in the copy at all) */
		const auto *hole = copy->getEventLabelIfPresent(lab.getPos());
		if (hole == nullptr || !genmc::isa<EmptyLabel>(hole))
			continue;
		auto inert = ReadLabel::create(
			lab.getPos(), MemOrdering::NotAtomic,
			SAddr::createStatic(SAddr::threadLimit, lab.getStamp().get(), false, true),
			ASize(1), copy->getInitLabel());
		copy->add(std::move(inert));
	}
	auto prefix = model.calculateRevisitPrefixView(copy->getEventLabel(sLab->getPos()));
	/* (RLAB is reached only as an event before a create or an end of its
	 * thread that SLAB's prefix has: then it is before SLAB whatever it
	 * reads) */
	if (prefix->contains(rLab->getPos()))
		return nullptr;

	/* RLAB in the model's prefix of SLAB only through the branches: through
	 * one on the way to an event of its thread after it that the prefix
	 * keeps (and whose own prefix has RLAB) */
	const auto dependents = dependentsOf(g, rLab, DependentSeeds::Scoped);
	if (!throughWrite && fullPrefix->contains(rLab->getPos())) {
		auto throughKept = false;
		for (auto i = rLab->getIndex() + 1;
		     !throughKept && i < g.getThreadSize(rLab->getThread()); i++) {
			const auto *kLab = g.getEventLabel(Event(rLab->getThread(), i));
			throughKept = !dependents[kLab->getStamp().get()] &&
				      prefix->contains(kLab->getPos()) &&
				      model.calculatePrefixView(kLab)->contains(rLab->getPos());
		}
		if (!throughKept)
			return nullptr;
	}
	auto &dprefix = *genmc::cast<DepView>(&*prefix);
	for (const auto &lab : g.labels())
		if (dependents[lab.getStamp().get()] && dprefix.contains(lab.getPos()))
			dprefix.addHole(lab.getPos());
	return prefix;
}

static void updatePredsWithPrefixView(const ExecutionGraph &g, VectorClock &preds,
				      const VectorClock &pporf)
{
	/* In addition to taking (preds U pporf), make sure pporf includes rfis */
	preds.update(pporf);

	if (!dynamic_cast<const DepExecutionGraph *>(&g))
		return;
	auto &predsD = *genmc::dyn_cast<DepView>(&preds);
	for (auto i = 0; i < pporf.size(); i++) {
		for (auto j = 1; j <= pporf.getMax(i); j++) {
			const auto *lab = g.getEventLabel(Event(i, j));
			if (const auto *rLab = genmc::dyn_cast<ReadLabel>(lab)) {
				if (preds.contains(rLab->getPos()) &&
				    !preds.contains(rLab->getRf()->getPos())) {
					if (rLab->getRf()->getThread() == rLab->getThread())
						predsD.removeHole(rLab->getRf()->getPos());
				}
			}
			const auto *wLab = genmc::dyn_cast<WriteLabel>(lab);
			if (wLab && wLab->isRMW() && pporf.contains(lab->getPos().prev()))
				predsD.removeHole(lab->getPos());
		}
	}
}

auto BackwardRevisits::revisitView(const ReadLabel *rLab, const WriteLabel *sLab,
				   bool *keptRMWReads) const
	-> std::unique_ptr<VectorClock>
{
	const auto &g = g_;
	auto preds = g.getPredsView(rLab->getPos());

	/* -exhaustive under -lkmm: the revisit keeps the model's prefix (the
	 * kat's rppo and rf: a kept event with its locks, fences and the
	 * same-location writes before it) without what depends on RLAB, which
	 * its new value recomputes (a branch's events, and what they ordered:
	 * the path-dependent part of the prefix, issue #77). What was in the
	 * prefix only through them stays: it does not depend on RLAB */
	if (opts_.keepsDependencies) {
		auto prefix = keptPrefixView(g, model_, rLab, sLab);
		if (!prefix)
			return nullptr;

		/* (The events before RLAB that exist because of what it read: kept
		 * events, revisited since) */
		const auto dependents = dependentsOf(g, rLab, DependentSeeds::Scoped);
		auto &dpreds = *genmc::cast<DepView>(&*preds);
		for (const auto &lab : g.labels())
			if (dependents[lab.getStamp().get()] && &lab != rLab &&
			    dpreds.contains(lab.getPos()))
				dpreds.addHole(lab.getPos());
		updatePredsWithPrefixView(g, *preds, *prefix);
		return keepRMWReads(rLab, sLab, std::move(preds), keptRMWReads);
	}

	/* A read in the write's prefix only through pctrl (-lkmm, see
	 * getRevisitPrefixView()): the prefix has events computed from the read's
	 * old value, so the revisit keeps the part of it without pctrl */
	const auto &prefix = getPrefixView(sLab);
	if (prefix.contains(rLab->getPos())) {
		updatePredsWithPrefixView(g, *preds, *getRevisitPrefixView(sLab));
		return keepRMWReads(rLab, sLab, std::move(preds), keptRMWReads);
	}
	updatePredsWithPrefixView(g, *preds, prefix);
	if (opts_.exhaustiveIMM) {
		/* (a CAS that fails reading SLAB loses its write part) */
		if (failsReading(rLab, sLab) && preds->contains(rLab->getPos().next()))
			genmc::cast<DepView>(&*preds)->addHole(rLab->getPos().next());
		if (!keepValueDependencies(rLab->getPos(), *preds))
			return nullptr;
	}
	return keepRMWReads(rLab, sLab, std::move(preds), keptRMWReads);
}

/* Whether the write part of the RMW RLAB is the same when RLAB reads from
 * SLAB: an xchg() writes its argument, a CAS its new value if it succeeds */
static auto writeIgnoresRead(const ReadLabel *rLab, const WriteLabel *sLab) -> bool
{
	if (const auto *fLab = genmc::dyn_cast<FaiReadLabel>(rLab))
		return fLab->getOp() == RMWBinOp::Xchg;
	return genmc::isa<CasReadLabel>(rLab) && !failsReading(rLab, sLab);
}

/* -exhaustive, without keepValueDependencies() (-lkmm). A kept read that
 * reads from a write of its own thread keeps that write (see
 * updatePredsWithPrefixView()), but not what the write depends on: for the
 * write part of an RMW, its read, whose value gives the write's. The read
 * may also have been revisited before, and so come after its write part in
 * the insertion order. A graph that keeps the write without its read is not
 * an execution of the program, and once it is cut between the two, the
 * revisits pushed for the read point at a hole (#223: a forward revisit of
 * the hole crashed in revisitRead()). The revisit keeps the read too, and,
 * transitively, the write it reads from (and, for an RMW's write, its
 * read), and the creation of their threads. The write part of the revisited
 * read itself is executed anew. The closure follows a kept read's write
 * even when the read is kept anyway, as one added before RLAB: it may since
 * have been revisited by a write added after RLAB (#237: the revisit was
 * otherwise refused as not closed, and whether a graph was reached
 * depended on the order the threads ran in). If a kept read reads from
 * the write part of RLAB, it stays when reading SLAB leaves its value as
 * it is (an xchg(), a CAS that still succeeds): it then moves right after
 * SLAB in coherence, as a write part kept while its read is executed anew
 * (updateKeptRMWWrite()). Otherwise
 * it is not kept, and the revisit is not taken if a kept read reads from it
 * (#237: under -lkmm, xchg()'s write is ordered after its read only by
 * rmw, so the kept read can be in SLAB's prefix while the xchg() is not) */
auto BackwardRevisits::keepRMWReads(const ReadLabel *rLab, const WriteLabel *sLab,
				    std::unique_ptr<VectorClock> v, bool *kept) const
	-> std::unique_ptr<VectorClock>
{
	if (!opts_.exhaustive || opts_.exhaustiveIMM || !opts_.depTracking)
		return v;

	const auto &g = g_;
	auto &dv = *genmc::cast<DepView>(&*v);
	auto isRMWWrite = [](const EventLabel *lab) {
		const auto *wLab = genmc::dyn_cast_if_present<WriteLabel>(lab);
		return wLab && wLab->isRMW();
	};
	const auto *own = g.po_imm_succ(rLab);
	auto keptOwn = false;
	if (isRMWWrite(own) && dv.contains(own->getPos())) {
		if (std::ranges::any_of(genmc::cast<WriteLabel>(own)->readers(), [&](auto &oLab) {
			    return dv.contains(oLab.getPos());
		    })) {
			if (!writeIgnoresRead(rLab, sLab))
				return nullptr;
			keptOwn = true;
		} else {
			dv.addHole(own->getPos());
		}
	}

	std::vector<Event> work;
	for (const auto &lab : g.labels())
		if (isRMWWrite(&lab) && dv.contains(lab.getPos()) && !(keptOwn && &lab == own))
			work.push_back(lab.getPos().prev());
	/* (The events it needs are followed even when the view has them: a
	 * read added before RLAB may since have been revisited by a write added
	 * after it, which it then also needs) */
	std::vector<char> seen(g.getMaxStamp().get() + 1, 0);
	while (!work.empty()) {
		auto e = work.back();
		work.pop_back();
		if (e.isInitializer())
			continue;
		const auto *lab = g.getEventLabel(e);
		if (std::exchange(seen[lab->getStamp().get()], 1))
			continue;
		if (dv.contains(e)) {
			if (e == rLab->getPos())
				continue;
		} else {
			if (e == rLab->getPos())
				return nullptr;
			dv.updateIdx(e);
			if (kept)
				*kept = true;
		}
		if (const auto *eRLab = genmc::dyn_cast<ReadLabel>(lab))
			work.push_back(eRLab->getRf()->getPos());
		if (isRMWWrite(lab))
			work.push_back(e.prev());
		const auto *bLab = genmc::cast<ThreadStartLabel>(g.getFirstThreadLabel(e.thread));
		if (bLab->getCreate())
			work.push_back(bLab->getCreate()->getPos());
	}
	return v;
}

/* -exhaustive under IMM. A revisit keeps the events added before the read and
 * the write's prefix, which is closed under (ppo | rfe)-predecessors; for a
 * kept read whose write is in its own thread, it also keeps the write (see
 * updatePredsWithPrefixView()), but not what the write depends on. That
 * write can be an RMW's write part whose read is not kept: its value is
 * then that of a read that is executed anew, and so is the value of the
 * kept read. The default exploration then updates the write when the read
 * is executed again (updateKeptRMWWrite()): it moves in coherence under the
 * readers it has, its revisits are queued with its old stamp (restricting
 * the graph to it cuts the read that completed it, and the revisiting
 * write), and its cached prefix view names events the graph no longer
 * has. Under -exhaustive, which takes revisits the maximality check
 * rejects, those cases end in crashes and in executions that are not
 * consistent or not executions of the program (issues #127, #135, #136).
 *
 * The reference keeps, instead, the graphs whose kept events are what the
 * program computes from the kept events: every event it keeps comes with
 * what its value and its existence depend on, its data, address, control
 * and CAS dependencies, an RMW's read for its write part, a read's write,
 * the creation of its thread, transitively. A revisit for which that
 * includes the revisited read, or an event the graph does not have, has no
 * such graph and is not taken. (A write is still kept without the events
 * it is merely ordered after: its value does not depend on them, and the
 * graph's consistency is checked once the revisit is done; see
 * backwardRevisit().) No kept write then changes when the threads fill
 * the holes, and none is completed anew */
auto BackwardRevisits::keepValueDependencies(Event r, VectorClock &v) const -> bool
{
	const auto &g = g_;
	auto &dv = *genmc::cast<DepView>(&v);
	/* (Every view contains the begins: a begin is not kept for its own sake
	 * and keeps nothing, its thread's kept events keep the create) */
	std::vector<Event> work;
	for (const auto &lab : g.labels())
		if (lab.getPos() != r && !genmc::isa<EmptyLabel>(&lab) &&
		    !genmc::isa<ThreadStartLabel>(&lab) && v.contains(lab.getPos()))
			work.push_back(lab.getPos());
	while (!work.empty()) {
		const auto *lab = g.getEventLabel(work.back());
		work.pop_back();

		std::vector<Event> deps;
		const auto &d = lab->getDeps();
		for (const auto *di : {&d.data, &d.addr, &d.ctrl, &d.cas})
			std::ranges::copy(*di, std::back_inserter(deps));
		if (const auto *wLab = genmc::dyn_cast<WriteLabel>(lab); wLab && wLab->isRMW())
			deps.push_back(lab->getPos().prev());
		if (const auto *rLab = genmc::dyn_cast<ReadLabel>(lab); rLab && rLab->getRf())
			deps.push_back(rLab->getRf()->getPos());
		if (const auto *bLab = g.getFirstThreadLabel(lab->getThread());
		    bLab && bLab->getCreate())
			deps.push_back(bLab->getCreate()->getPos());

		for (auto e : deps) {
			if (e == r)
				return false;
			if (e.isInitializer() || v.contains(e))
				continue;
			const auto *eLab = g.getEventLabelIfPresent(e);
			if (!eLab || genmc::isa<EmptyLabel>(eLab))
				return false;
			dv.updateIdx(e);
			work.push_back(e);
		}
	}
	return true;
}

auto BackwardRevisits::construct(const ReadLabel *rLab, const WriteLabel *sLab) const
	-> std::unique_ptr<BackwardRevisit>
{
	return std::make_unique<BackwardRevisit>(rLab, sLab, revisitView(rLab, sLab));
}

static auto isReadInView(const WriteLabel *sLab, const DepView &v) -> bool
{
	return std::ranges::any_of(sLab->readers(),
				   [&v](auto &rLab) { return v.contains(rLab.getPos()); });
}

auto BackwardRevisits::isCoBeforeSavedPrefix(const BackwardRevisit &r, const EventLabel *lab) const -> bool
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
auto BackwardRevisits::isPlacedMaximallyInRevisit(const BackwardRevisit &r,
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
			/* (An RMW read that reading BOUND makes succeed may not:
			 * its write would go right after BOUND in coherence, which
			 * QLAB would read before. See ChoiceCandidates::rfs()) */
			mayReadBound = !rLab || !rLab->valueMakesRMWSucceed(bound->getAccessValue(
							rLab->getAccess()));
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
auto BackwardRevisits::isCoAfterLaterSavedWrite(const BackwardRevisit &r, const EventLabel *lab) const -> bool
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

auto BackwardRevisits::coherenceSuccRemainInGraph(const BackwardRevisit &r) const -> bool
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
auto BackwardRevisits::isKeptByKeptRevisit(const BackwardRevisit &r, const EventLabel *lab) const -> bool
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

/* A read revisited although it did not read the maximal write when it was
 * added (see getRevisitable()) may be bounded by an access of its thread
 * after it that the revisit keeps: the revisiting write must not be co-after
 * the write that access reads, or co-after or equal to the write it is (an
 * RMW read's own write part moves with it, and is no bound) */
auto BackwardRevisits::breaksCoherenceOfKeptLater(const BackwardRevisit &r) const -> bool
{
	const auto &g = g_;
	const auto &v = r.getViewNoRel();
	const auto *rLab = g.getReadLabel(r.getPos());
	const auto *sLab = g.getWriteLabel(r.getRev());
	const auto *ownLab = rLab->isRMW() ? g.getWriteLabel(rLab->getPos().next()) : nullptr;
	auto coBefore = [&](const EventLabel *a, const EventLabel *b) {
		if (a == b || genmc::isa<InitLabel>(b))
			return false;
		if (genmc::isa<InitLabel>(a))
			return true;
		return std::ranges::any_of(g.co_succs(genmc::cast<WriteLabel>(a)),
					   [&](auto &wLab) { return &wLab == b; });
	};
	for (auto j = rLab->getIndex() + 1; j < g.getThreadSize(rLab->getThread()); j++) {
		const auto *mLab = genmc::dyn_cast<MemAccessLabel>(
			g.getEventLabel(Event(rLab->getThread(), j)));
		if (!mLab || mLab == ownLab || mLab->getAddr() != rLab->getAddr() ||
		    !v->contains(mLab->getPos()))
			continue;
		if (const auto *qLab = genmc::dyn_cast<ReadLabel>(mLab)) {
			if (!qLab->getRf() || qLab->getRf() == ownLab)
				continue;
			return coBefore(qLab->getRf(), sLab);
		}
		return !coBefore(sLab, mLab);
	}
	return false;
}

auto BackwardRevisits::isMaximalExtension(const BackwardRevisit &r) const -> bool
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
auto BackwardRevisits::isRevisitViewClosed(const BackwardRevisit &r) const -> bool
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
auto BackwardRevisits::keepsConflictingRMW(const BackwardRevisit &r) const -> bool
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

