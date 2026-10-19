/*
 * GenMC -- Generic Model Checking.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * The relations of the Linux-kernel memory model (the kernel's
 * tools/memory-model/linux-kernel.cat) that Kater cannot generate for
 * LKMMChecker: (prop \ id) & int, the part of propagation that comes back
 * to the thread it started from. The definitions, with Marked the atomic
 * events (GenMC's non-atomic accesses are LKMM's plain ones) and M the
 * memory accesses:
 *
 *   po-rel = [M] ; po ; [Release]
 *   po-unlock-lock-po = po ; [UL] ; (po|rf) ; [LKR] ; po
 *   wmb = [W] ; po ; [Wmb] ; po ; [W]
 *   mb = [M] ; po ; [Mb] ; po ; [M] | [M] ; po ; [Mb & R] | [Mb & W] ; po ; [M]
 *      | [M] ; po ; [Before-atomic] ; po ; [RMW] ; po? ; [M]
 *      | [M] ; po? ; [RMW] ; po ; [After-atomic] ; po ; [M]
 *      | [M] ; po? ; [LKW] ; po ; [After-spinlock] ; po ; [M]
 *      | [M] ; po-unlock-lock-po ; [After-unlock-lock] ; po ; [M]
 *   gp = po ; [Sync-rcu] ; po?                       (here [M] ; po ; [Sync-rcu] ; po ; [M])
 * where, in mb and gp, M also has GenMC's thread events (creation and join
 * synchronise like a release and an acquire, which a strong fence reaches)
 *   strong-fence = mb | gp
 *   A-cumul(r) = (rfe ; [Marked])? ; r
 *   rmw-sequence = (rf ; rmw)*
 *   cumul-fence = [Marked] ; (A-cumul(strong-fence | po-rel) | wmb
 *                             | po-unlock-lock-po) ; [Marked] ; rmw-sequence
 *   prop = [Marked] ; (overwrite & ext)? ; cumul-fence* ; [Marked] ; rfe? ; [Marked]
 *
 * The computation runs backwards from the label: the events cumul-fence*
 * before it (or before the external write it reads from), then one step of
 * coe/fre back into the label's thread. The predicates agree with the ones
 * of kat/lkmm-genmc.kat (the Kater fork), which generates the rest.
 */

#include "genmc/Execution/LkmmRelations.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Support/Cast.hpp"

#include <unordered_map>
#include <vector>

namespace {

using Labels = std::vector<const EventLabel *>;

auto isMarked(const EventLabel *lab) -> bool { return !lab->isNotAtomic(); }
auto isMem(const EventLabel *lab) -> bool { return genmc::isa<MemAccessLabel>(lab); }
auto isWrite(const EventLabel *lab) -> bool { return genmc::isa<WriteLabel>(lab); }

/* An endpoint of a strong fence: an access or a thread event (creation and
 * join synchronise like a release and an acquire) */
auto isMemOrThread(const EventLabel *lab) -> bool
{
	return isMem(lab) || genmc::isa<ThreadCreateLabel>(lab) ||
	       genmc::isa<ThreadFinishLabel>(lab) || genmc::isa<ThreadStartLabel>(lab) ||
	       genmc::isa<ThreadJoinLabel>(lab);
}

auto isFence(const EventLabel *lab, LkmmFenceKind kind) -> bool
{
	const auto *fLab = genmc::dyn_cast<LkmmFenceLabel>(lab);
	return fLab && fLab->getFenceKind() == kind;
}

/* The bell's Release = RELEASE \ R \ FailedRMW: the kernel has no release
 * fence (smp_wmb() is a Wmb fence; the ordering it carries is for GenMC) */
auto isRelease(const EventLabel *lab) -> bool
{
	return isWrite(lab) && lab->isAtLeastRelease();
}

/* An access of an RMW that took effect (a failed cmpxchg() is a plain read) */
auto isRMWAccess(const EventLabel *lab) -> bool
{
	if (const auto *rLab = genmc::dyn_cast<ReadLabel>(lab))
		return rLab->isRMW();
	if (const auto *wLab = genmc::dyn_cast<WriteLabel>(lab))
		return wLab->isRMW();
	return false;
}

auto isLKR(const EventLabel *lab) -> bool
{
	return (genmc::isa<LockCasReadLabel>(lab) || genmc::isa<TrylockCasReadLabel>(lab)) &&
	       genmc::cast<ReadLabel>(lab)->isRMW();
}

auto isLKW(const EventLabel *lab) -> bool
{
	return genmc::isa<LockCasWriteLabel>(lab) || genmc::isa<TrylockCasWriteLabel>(lab);
}

auto isUL(const EventLabel *lab) -> bool { return genmc::isa<UnlockWriteLabel>(lab); }

/* All events po-before LAB that satisfy PRED, added to OUT */
template <typename F> void poPredsIf(const ExecutionGraph &g, const EventLabel *lab, F pred, Labels &out)
{
	for (const auto &pLab : g.po_preds(lab))
		if (pred(&pLab))
			out.push_back(&pLab);
}

/* po-unlock-lock-po: the unlocks u with u ; (po|rf) ; L for a lock read L */
auto unlocksBefore(const ExecutionGraph &g, const EventLabel *lkr) -> Labels
{
	Labels uls;
	poPredsIf(g, lkr, isUL, uls);
	const auto *rf = genmc::cast<ReadLabel>(lkr)->getRf();
	if (rf && isUL(rf) && rf->getThread() != lkr->getThread())
		uls.push_back(rf);
	return uls;
}

/* The y with (y, x) in mb | gp: the fences, grace periods and fully ordered
 * RMWs that stand between y and x in x's thread, plus the lock handover of
 * smp_mb__after_unlock_lock() and the srcu_read_unlock() of
 * smp_mb__after_srcu_read_unlock() */
auto mbPreds(const ExecutionGraph &g, const EventLabel *x) -> Labels
{
	Labels out;
	if (!isMemOrThread(x))
		return out;

	/* [M] ; po ; [Mb & R] */
	if (genmc::isa<ReadLabel>(x) && x->isLkmmMb())
		poPredsIf(g, x, isMemOrThread, out);

	/* Scan back from x; the flags say what stood between the current event and x */
	bool sawMb = false;	    /* [M] ; po ; [Mb | Sync-rcu | Sync-srcu] ; po ; [M] */
	bool sawRMW = isRMWAccess(x); /* ... ; [Before-atomic] ; po ; [RMW] ; po? ; [M] */
	bool sawBa = false;
	bool sawAa = false;	    /* [M] ; po? ; [RMW] ; po ; [After-atomic] ; po ; [M] */
	bool sawRMWBeforeAa = false;
	bool sawAs = false;	    /* [M] ; po? ; [LKW] ; po ; [After-spinlock] ; po ; [M] */
	bool sawLKWBeforeAs = false;
	bool sawAsru = false; /* [M] ; po? ; [Srcu-unlock] ; po ; [After-srcu-read-unlock] ; po ; [M] */
	bool sawSrcuUBeforeAsru = false;
	for (const auto &pLab : g.po_preds(x)) {
		const auto *y = &pLab;
		if (isFence(y, LkmmFenceKind::Mb) || isFence(y, LkmmFenceKind::SyncRcu) ||
		    isFence(y, LkmmFenceKind::SyncSrcu))
			sawMb = true;
		if (isFence(y, LkmmFenceKind::AfterSrcuReadUnlock))
			sawAsru = true;
		if (genmc::isa<SrcuUnlockWriteLabel>(y) && sawAsru)
			sawSrcuUBeforeAsru = true;
		if (isFence(y, LkmmFenceKind::BeforeAtomic) && sawRMW)
			sawBa = true;
		if (isFence(y, LkmmFenceKind::AfterAtomic))
			sawAa = true;
		if (isFence(y, LkmmFenceKind::AfterSpinlock))
			sawAs = true;
		if (isRMWAccess(y)) {
			sawRMW = true;
			if (sawAa)
				sawRMWBeforeAa = true;
		}
		if (isLKW(y) && sawAs)
			sawLKWBeforeAs = true;
		/* [M] ; po-unlock-lock-po ; [After-unlock-lock] ; po ; [M] */
		if (isFence(y, LkmmFenceKind::AfterUnlockLock)) {
			for (const auto &lLab : g.po_preds(y))
				if (isLKR(&lLab))
					for (const auto *ul : unlocksBefore(g, &lLab))
						poPredsIf(g, ul, isMemOrThread, out);
		}
		if (!isMemOrThread(y))
			continue;
		if (sawMb || sawBa || sawRMWBeforeAa || sawLKWBeforeAs || sawSrcuUBeforeAsru ||
		    (isWrite(y) && y->isLkmmMb())) /* [Mb & W] ; po ; [M] */
			out.push_back(y);
	}
	return out;
}

/* The y with (y, x) in po-rel */
auto poRelPreds(const ExecutionGraph &g, const EventLabel *x) -> Labels
{
	Labels out;
	if (isRelease(x))
		poPredsIf(g, x, isMem, out);
	return out;
}

/* The y with (y, x) in wmb */
auto wmbPreds(const ExecutionGraph &g, const EventLabel *x) -> Labels
{
	Labels out;
	if (!isWrite(x))
		return out;
	bool sawWmb = false;
	for (const auto &pLab : g.po_preds(x)) {
		if (isFence(&pLab, LkmmFenceKind::Wmb))
			sawWmb = true;
		else if (sawWmb && isWrite(&pLab))
			out.push_back(&pLab);
	}
	return out;
}

/* The y with (y, x) in po-unlock-lock-po */
auto poUnlockLockPoPreds(const ExecutionGraph &g, const EventLabel *x) -> Labels
{
	Labels out;
	for (const auto &lLab : g.po_preds(x))
		if (isLKR(&lLab))
			for (const auto *ul : unlocksBefore(g, &lLab))
				poPredsIf(g, ul, [](const EventLabel *) { return true; }, out);
	return out;
}

/* rmw-sequence^-1: x itself and the writes an RMW chain ending in x read from */
auto rmwSequenceSources(const ExecutionGraph &g, const EventLabel *x) -> Labels
{
	Labels out{x};
	const auto *cur = x;
	while (const auto *wLab = genmc::dyn_cast<WriteLabel>(cur)) {
		if (!wLab->isRMW())
			break;
		const auto *rLab = genmc::dyn_cast_if_present<ReadLabel>(g.po_imm_pred(wLab));
		if (!rLab || !rLab->getRf() || genmc::isa<InitLabel>(rLab->getRf()))
			break;
		cur = rLab->getRf();
		out.push_back(cur);
	}
	return out;
}

/* The y with (y, x) in cumul-fence, rmw-sequence undone by the caller */
auto cumulFenceStepPreds(const ExecutionGraph &g, const EventLabel *x) -> Labels
{
	Labels out;
	if (!isMarked(x))
		return out;

	/* A-cumul(strong-fence | po-rel): also the external writes the reads among
	 * the predecessors read from */
	auto cumul = mbPreds(g, x);
	for (const auto *y : poRelPreds(g, x))
		cumul.push_back(y);
	for (const auto *y : cumul) {
		if (!isMarked(y))
			continue;
		out.push_back(y);
		if (const auto *w = g.rfe_pred(y); w && !genmc::isa<InitLabel>(w))
			out.push_back(w);
	}
	for (const auto *y : wmbPreds(g, x))
		out.push_back(y);
	for (const auto *y : poUnlockLockPoPreds(g, x))
		out.push_back(y);
	return out;
}

} // namespace

namespace {
/* The results of propIntPreds() while a PropIntCache lives, by stamp */
thread_local std::unordered_map<unsigned, Labels> *propIntCache = nullptr;
auto computePropIntPreds(const ExecutionGraph &g, const EventLabel *lab) -> Labels;
} // namespace

lkmm::PropIntCache::PropIntCache() : owner(propIntCache == nullptr)
{
	if (owner)
		propIntCache = new std::unordered_map<unsigned, Labels>();
}

lkmm::PropIntCache::~PropIntCache()
{
	if (owner) {
		delete propIntCache;
		propIntCache = nullptr;
	}
}

auto lkmm::propIntPreds(const ExecutionGraph &g, const EventLabel *lab) -> Labels
{
	if (!propIntCache)
		return computePropIntPreds(g, lab);
	auto [it, inserted] = propIntCache->try_emplace(lab->getStamp().get());
	if (inserted)
		it->second = computePropIntPreds(g, lab);
	return it->second;
}

namespace {
auto computePropIntPreds(const ExecutionGraph &g, const EventLabel *lab) -> Labels
{
	Labels result;
	if (!isMarked(lab))
		return result;

	/* C: the Marked events cumul-fence*-before lab or before the external write it reads */
	std::vector<char> inC(g.getMaxStamp().get() + 1, 0);
	Labels work;
	auto add = [&](const EventLabel *x) {
		if (!x || genmc::isa<InitLabel>(x) || !isMarked(x) || inC[x->getStamp().get()])
			return;
		inC[x->getStamp().get()] = 1;
		work.push_back(x);
	};
	add(lab);
	add(g.rfe_pred(lab));
	while (!work.empty()) {
		const auto *x = work.back();
		work.pop_back();
		for (const auto *src : rmwSequenceSources(g, x))
			for (const auto *y : cumulFenceStepPreds(g, src))
				add(y);
	}

	/* (overwrite & ext)^-1 into lab's thread: per location, the co-latest
	 * write of C in another thread; the thread's writes co-before it and its
	 * reads of a write co-before it (or of the initial value) are prop-before lab */
	const auto tid = lab->getThread();
	std::unordered_map<SAddr, const WriteLabel *> coLatest;
	std::unordered_map<SAddr, std::unordered_map<const EventLabel *, int>> coPos;
	auto posOf = [&](const WriteLabel *w) -> int {
		auto &pos = coPos[w->getAddr()];
		if (pos.empty()) {
			auto i = 0;
			for (const auto &cLab : g.co(w->getAddr()))
				pos[&cLab] = i++;
		}
		return pos.at(w);
	};
	for (const auto &cLab : g.labels()) {
		const auto *w = genmc::dyn_cast<WriteLabel>(&cLab);
		if (!w || !inC[w->getStamp().get()] || w->getThread() == tid)
			continue;
		auto [it, inserted] = coLatest.try_emplace(w->getAddr(), w);
		if (!inserted && posOf(w) > posOf(it->second))
			it->second = w;
	}

	for (auto i = 1; i < g.getThreadSize(tid); i++) {
		const auto *e = g.getEventLabel(Event(tid, i));
		if (e == lab || !isMarked(e))
			continue;
		if (inC[e->getStamp().get()]) {
			result.push_back(e);
			continue;
		}
		if (const auto *w = genmc::dyn_cast<WriteLabel>(e)) {
			auto it = coLatest.find(w->getAddr());
			if (it != coLatest.end() && posOf(w) < posOf(it->second))
				result.push_back(e);
		} else if (const auto *r = genmc::dyn_cast<ReadLabel>(e)) {
			auto it = coLatest.find(r->getAddr());
			if (it == coLatest.end() || !r->getRf())
				continue;
			const auto *rf = genmc::dyn_cast<WriteLabel>(r->getRf());
			if (!rf || posOf(rf) < posOf(it->second))
				result.push_back(e);
		}
	}
	return result;
}
} // namespace

auto ExecutionGraph::prop_int_preds(const EventLabel *lab) const -> std::vector<const EventLabel *>
{
	return lkmm::propIntPreds(*this, lab);
}
