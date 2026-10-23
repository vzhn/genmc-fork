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
 * The Linux-kernel memory model (the kernel's tools/memory-model/
 * linux-kernel.cat) evaluated on a whole execution graph.
 * The events are the graph's labels but the initial one (nothing precedes
 * it, so it lies on no cycle); Marked is GenMC's atomic labels, fences and
 * RCU events included, as in herd7. GenMC's additions are those of
 * kat/lkmm-genmc.kat in the Kater fork: dependencies reach the write of an
 * RMW (rmw?), and thread creation and join order (thread-po, tc, tj).
 *
 * RCU, as the cat file has it:
 *
 *   rcu-rscs: the nested rcu_read_lock()/rcu_read_unlock() pairs of a thread
 *   srcu-rscs = ([Srcu-lock] ; carry-srcu-data ; data ; [Srcu-unlock]) & loc,
 *       carry-srcu-data = (data ; [~Srcu-unlock] ; rf)*: an srcu_read_unlock()
 *       matches the srcu_read_lock() whose value it writes back
 *   rcu-link = po? ; hb* ; pb* ; prop ; po
 *   rcu-order = the least fixpoint of
 *       rcu-gp | srcu-gp
 *     | (rcu-gp ; rcu-link ; rcu-rscsi) | (rcu-rscsi ; rcu-link ; rcu-gp)
 *     | (rcu-gp ; rcu-link ; rcu-order ; rcu-link ; rcu-rscsi)
 *     | (rcu-rscsi ; rcu-link ; rcu-order ; rcu-link ; rcu-gp)
 *     | the same four with srcu-gp and srcu-rscsi, & loc (one srcu_struct)
 *     | (rcu-order ; rcu-link ; rcu-order)
 *   rcu-fence = po ; rcu-order ; po?
 *   rb = prop ; rcu-fence ; hb* ; pb* ; [Marked]          irreflexive rb
 *
 * fence and strong-fence also gain rcu-fence after rb, but only the
 * plain-access rules use those. SRCU's events are accesses of their
 * srcu_struct (a lock reads it, an unlock writes the index back), but for
 * synchronize_srcu(), a fence with the srcu_struct as its location.
 *
 * Plain accesses, as the cat file has them (xbstar = (hb | pb | rb)*):
 *
 *   vis = cumul-fence* ; rfe? ; [Marked] ;
 *         ((strong-fence ; [Marked] ; xbstar) | (xbstar & int))
 *   w-pre-bounded, r-pre-bounded, w-post-bounded, r-post-bounded: a plain
 *         access's fence or address dependency to a marked one
 *   ww-vis, wr-vis, rw-xbstar: what orders two conflicting accesses
 *   pre-race = ext & ((Plain * M) | ((M \ IW) * Plain))
 *   plain-coherence: empty (wr-incoh | rw-incoh | ww-incoh), an axiom
 *   data-race: ww-race | wr-race | rw-race, a flag (GenMC's error)
 *
 * GenMC's thread creation and join count as a release write read by an
 * acquire read, and the pairs they order are no potential races.
 */

#include "genmc/Models/LkmmModel.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Support/Cast.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

/* A set of events, by index */
using Set = std::vector<char>;

/* A binary relation over n events, a bit matrix */
class Rel {
public:
	explicit Rel(size_t n) : n_(n), words_((n + 63) / 64), bits_(n * words_, 0) {}

	[[nodiscard]] auto size() const -> size_t { return n_; }

	void set(size_t i, size_t j) { row(i)[j / 64] |= (uint64_t(1) << (j % 64)); }
	/* Adds (i, j) for every j in [lo, hi) */
	void setRange(size_t i, size_t lo, size_t hi)
	{
		for (auto j = lo; j < hi; j++)
			set(i, j);
	}
	[[nodiscard]] auto get(size_t i, size_t j) const -> bool
	{
		return (row(i)[j / 64] >> (j % 64)) & 1U;
	}

	auto operator|=(const Rel &o) -> Rel &
	{
		for (auto k = 0U; k < bits_.size(); k++)
			bits_[k] |= o.bits_[k];
		return *this;
	}
	friend auto operator|(Rel a, const Rel &b) -> Rel { return a |= b; }
	friend auto operator&(Rel a, const Rel &b) -> Rel
	{
		for (auto k = 0U; k < a.bits_.size(); k++)
			a.bits_[k] &= b.bits_[k];
		return a;
	}
	friend auto operator-(Rel a, const Rel &b) -> Rel
	{
		for (auto k = 0U; k < a.bits_.size(); k++)
			a.bits_[k] &= ~b.bits_[k];
		return a;
	}
	auto operator==(const Rel &o) const -> bool { return bits_ == o.bits_; }

	/* this ; o */
	[[nodiscard]] auto seq(const Rel &o) const -> Rel
	{
		Rel r(n_);
		for (auto i = 0U; i < n_; i++)
			forEach(i, [&](size_t k) {
				for (auto w = 0U; w < words_; w++)
					r.row(i)[w] |= o.row(k)[w];
			});
		return r;
	}

	static auto id(const Set &s) -> Rel
	{
		Rel r(s.size());
		for (auto i = 0U; i < s.size(); i++)
			if (s[i])
				r.set(i, i);
		return r;
	}

	/* [d] ; this ; [c] */
	[[nodiscard]] auto restrict(const Set &d, const Set &c) const -> Rel
	{
		std::vector<uint64_t> mask(words_, 0);
		for (auto j = 0U; j < n_; j++)
			if (c[j])
				mask[j / 64] |= uint64_t(1) << (j % 64);
		Rel r(n_);
		for (auto i = 0U; i < n_; i++)
			if (d[i])
				for (auto w = 0U; w < words_; w++)
					r.row(i)[w] = row(i)[w] & mask[w];
		return r;
	}
	[[nodiscard]] auto dom(const Set &d) const -> Rel { return restrict(d, all()); }
	[[nodiscard]] auto range(const Set &c) const -> Rel { return restrict(all(), c); }

	[[nodiscard]] auto inverse() const -> Rel
	{
		Rel r(n_);
		for (auto i = 0U; i < n_; i++)
			forEach(i, [&](size_t j) { r.set(j, i); });
		return r;
	}

	/* this+ (Warshall; a node without successors extends no path) */
	[[nodiscard]] auto plus() const -> Rel
	{
		Rel r = *this;
		for (auto k = 0U; k < n_; k++) {
			if (r.rowEmpty(k))
				continue;
			const auto kw = k / 64;
			const auto kb = uint64_t(1) << (k % 64);
			for (auto i = 0U; i < n_; i++)
				if (r.row(i)[kw] & kb)
					for (auto w = 0U; w < words_; w++)
						r.row(i)[w] |= r.row(k)[w];
		}
		return r;
	}
	[[nodiscard]] auto star() const -> Rel { return plus().opt(); }
	[[nodiscard]] auto opt() const -> Rel
	{
		Rel r = *this;
		for (auto i = 0U; i < n_; i++)
			r.set(i, i);
		return r;
	}

	/* Calls F on each j with (i, j) */
	template <typename F> void forEachSucc(size_t i, F f) const { forEach(i, f); }
	/* Calls F on each pair */
	template <typename F> void forAll(F f) const
	{
		for (auto i = 0U; i < n_; i++)
			forEach(i, [&](size_t j) { f(i, j); });
	}

	[[nodiscard]] auto irreflexive() const -> bool
	{
		for (auto i = 0U; i < n_; i++)
			if (get(i, i))
				return false;
		return true;
	}
	[[nodiscard]] auto acyclic() const -> bool { return plus().irreflexive(); }
	[[nodiscard]] auto empty() const -> bool
	{
		for (auto b : bits_)
			if (b)
				return false;
		return true;
	}

private:
	[[nodiscard]] auto rowEmpty(size_t i) const -> bool
	{
		const auto *r = row(i);
		for (auto w = 0U; w < words_; w++)
			if (r[w])
				return false;
		return true;
	}
	[[nodiscard]] auto all() const -> Set { return Set(n_, 1); }
	auto row(size_t i) -> uint64_t * { return &bits_[i * words_]; }
	[[nodiscard]] auto row(size_t i) const -> const uint64_t * { return &bits_[i * words_]; }

	template <typename F> void forEach(size_t i, F f) const
	{
		const auto *r = row(i);
		for (auto w = 0U; w < words_; w++)
			for (auto b = r[w]; b; b &= b - 1)
				f(w * 64 + __builtin_ctzll(b));
	}

	size_t n_;
	size_t words_;
	std::vector<uint64_t> bits_;
};

auto fenceKind(const EventLabel *lab) -> std::optional<LkmmFenceKind>
{
	if (const auto *fLab = genmc::dyn_cast<LkmmFenceLabel>(lab))
		return fLab->getFenceKind();
	return std::nullopt;
}

auto isFence(const EventLabel *lab, LkmmFenceKind kind) -> bool
{
	return fenceKind(lab) == kind;
}

auto isRMWAccess(const EventLabel *lab) -> bool
{
	if (const auto *rLab = genmc::dyn_cast<ReadLabel>(lab))
		return rLab->isRMW();
	if (const auto *wLab = genmc::dyn_cast<WriteLabel>(lab))
		return wLab->isRMW();
	return false;
}

/* The srcu_struct of an SRCU event (herd7's location of the event) */
auto srcuAddr(const EventLabel *lab) -> std::optional<SAddr>
{
	if (genmc::isa<SrcuLockReadLabel>(lab) || genmc::isa<SrcuUnlockWriteLabel>(lab))
		return genmc::cast<MemAccessLabel>(lab)->getAddr();
	if (isFence(lab, LkmmFenceKind::SyncSrcu))
		return genmc::cast<LkmmFenceLabel>(lab)->getSrcuAddr();
	return std::nullopt;
}

/* The y with (y, x) in GenMC's data ; rmw?: the reads whose values X's
 * value depends on (the write of an RMW has its dependencies on its read) */
template <typename F> void forEachDataPred(const ExecutionGraph &g, const EventLabel *x, F f)
{
	for (auto e : x->getDeps().data)
		f(e);
	if (const auto *wLab = genmc::dyn_cast<WriteLabel>(x); wLab && wLab->isRMW())
		if (const auto *rLab = g.po_imm_pred(wLab))
			for (auto e : rLab->getDeps().data)
				f(e);
}

} // namespace

auto lkmm::srcuLocksOf(const ExecutionGraph &g, const EventLabel *unlock)
	-> std::vector<const EventLabel *>
{
	std::vector<const EventLabel *> locks;
	const auto *u = genmc::dyn_cast<SrcuUnlockWriteLabel>(unlock);
	if (!u)
		return locks;
	/* Back from U by data ; ([~Srcu-unlock] ; rf ; data)* */
	std::vector<Event> work;
	std::unordered_set<Event> seen;
	auto push = [&](Event e) {
		if (seen.insert(e).second)
			work.push_back(e);
	};
	forEachDataPred(g, u, push);
	while (!work.empty()) {
		const auto *lab = g.getEventLabelIfPresent(work.back());
		work.pop_back();
		/* GenMC's thread creation and join pass a value as a write read
		 * by a read would: a thread's argument depends on its start, the
		 * create on the argument; a joined value on the join, the
		 * thread's end on the value it returns */
		if (const auto *bLab = genmc::dyn_cast_if_present<ThreadStartLabel>(lab)) {
			if (const auto *cLab = bLab->getCreate())
				forEachDataPred(g, cLab, push);
			continue;
		}
		if (const auto *jLab = genmc::dyn_cast_if_present<ThreadJoinLabel>(lab)) {
			if (const auto *eLab = genmc::dyn_cast_if_present<ThreadFinishLabel>(
				    g.getLastThreadLabel(jLab->getChildId())))
				forEachDataPred(g, eLab, push);
			continue;
		}
		const auto *r = genmc::dyn_cast_if_present<ReadLabel>(lab);
		if (!r)
			continue;
		if (genmc::isa<SrcuLockReadLabel>(r) && r->getAddr() == u->getAddr())
			locks.push_back(r);
		const auto *w = genmc::dyn_cast_if_present<WriteLabel>(r->getRf());
		if (w && !genmc::isa<SrcuUnlockWriteLabel>(w))
			forEachDataPred(g, w, push);
	}
	return locks;
}

auto lkmm::srcuRscs(const ExecutionGraph &g)
	-> std::vector<std::pair<const EventLabel *, const EventLabel *>>
{
	std::vector<std::pair<const EventLabel *, const EventLabel *>> pairs;
	for (const auto &lab : g.labels())
		for (const auto *l : srcuLocksOf(g, &lab))
			pairs.emplace_back(l, &lab);
	return pairs;
}

auto lkmm::srcuOwnGracePeriod(const ExecutionGraph &g, const EventLabel *unlock)
	-> const EventLabel *
{
	const auto *u = genmc::dyn_cast<SrcuUnlockWriteLabel>(unlock);
	if (!u)
		return nullptr;
	for (const auto *l : srcuLocksOf(g, u)) {
		if (l->getThread() != u->getThread())
			continue;
		for (auto i = l->getIndex() + 1; i < u->getIndex(); i++) {
			const auto *lab = g.getEventLabelIfPresent(Event(u->getThread(), i));
			if (isFence(lab, LkmmFenceKind::SyncSrcu) &&
			    genmc::cast<LkmmFenceLabel>(lab)->getSrcuAddr() == u->getAddr())
				return lab;
		}
	}
	return nullptr;
}

auto lkmm::findSrcuMisuse(const ExecutionGraph &g) -> std::optional<SrcuMisuse>
{
	auto pairs = srcuRscs(g);
	std::unordered_map<const EventLabel *, const EventLabel *> unlockOf;
	std::unordered_set<const EventLabel *> matchedUnlocks;
	for (auto [l, u] : pairs) {
		/* multiple-srcu-matches */
		if (auto [it, fresh] = unlockOf.emplace(l, u); !fresh)
			return SrcuMisuse{u, it->second,
					  "srcu_read_lock() matched by more than one "
					  "srcu_read_unlock()"};
		matchedUnlocks.insert(u);
		/* srcu-bad-value-match */
		if (genmc::cast<ReadLabel>(l)->getReturnValue() !=
		    genmc::cast<WriteLabel>(u)->getVal())
			return SrcuMisuse{u, l,
					  "srcu_read_unlock() of another value than its "
					  "srcu_read_lock() returned"};
	}
	for (const auto &lab : g.labels()) {
		if (genmc::isa<SrcuUnlockWriteLabel>(&lab) && !matchedUnlocks.contains(&lab))
			return SrcuMisuse{&lab, nullptr,
					  "srcu_read_unlock() without a matching srcu_read_lock()"};
		if (genmc::isa<SrcuLockReadLabel>(&lab) && !unlockOf.contains(&lab))
			return SrcuMisuse{&lab, nullptr,
					  "srcu_read_lock() without a matching srcu_read_unlock()"};
	}
	return std::nullopt;
}

namespace {

/* The events and base relations of a graph. With CLOSEOPEN, each
 * rcu_read_lock() or srcu_read_lock() still unmatched gets a virtual unlock
 * at the end of its thread (a node without a label) */
class Model {
public:
	/* With PRUNE, the nodes are only those that can matter to rb (see
	 * mattersToRb()); coherence and the freed-memory ordering need all */
	Model(const ExecutionGraph &g, bool closeOpen, bool prune = false) : g(g)
	{
		auto srcuPairs = lkmm::srcuRscs(g);
		std::unordered_set<const EventLabel *> srcuMatched;
		for (auto [l, u] : srcuPairs)
			srcuMatched.insert(l);

		/* By thread, in po order: a thread's events are [begin, end) */
		for (auto t = 0U; t < g.getNumThreads(); t++) {
			auto begin = labs.size();
			std::vector<size_t> open;
			std::vector<size_t> openSrcu;
			for (auto k = 0; k < g.getThreadSize(t); k++) {
				const auto *lab = g.getEventLabelIfPresent(Event(t, k));
				if (!isNode(lab) || (prune && !mattersToRb(lab)))
					continue;
				addNode(lab, srcuAddr(lab));
				if (genmc::isa<SrcuLockReadLabel>(lab) && !srcuMatched.contains(lab))
					openSrcu.push_back(labs.size() - 1);
				/* rcu-rscs: innermost first, as the bell's fixpoint matches */
				if (isFence(lab, LkmmFenceKind::RcuLock)) {
					open.push_back(labs.size() - 1);
				} else if (isFence(lab, LkmmFenceKind::RcuUnlock) && !open.empty()) {
					rscs.emplace_back(open.back(), labs.size() - 1);
					open.pop_back();
				}
			}
			for (; closeOpen && !open.empty(); open.pop_back()) {
				rscs.emplace_back(open.back(), labs.size());
				addNode(nullptr);
			}
			for (; closeOpen && !openSrcu.empty(); openSrcu.pop_back()) {
				srcuRscsPairs.emplace_back(openSrcu.back(), labs.size());
				addNode(nullptr, srcuLocs[openSrcu.back()]);
			}
			for (auto i = begin; i < labs.size(); i++)
				threadEnd.push_back(labs.size());
			for (auto i = begin; i < labs.size(); i++)
				threadBegin.push_back(begin);
		}
		for (auto [l, u] : srcuPairs)
			if (auto i = idx(l), j = idx(u); i >= 0 && j >= 0)
				srcuRscsPairs.emplace_back(i, j);
	}

	[[nodiscard]] auto size() const -> size_t { return labs.size(); }

	/* The events that satisfy PRED (never a virtual one) */
	template <typename P> [[nodiscard]] auto set(P pred) const -> Set
	{
		Set s(size(), 0);
		for (auto i = 0U; i < size(); i++)
			s[i] = (labs[i] && pred(labs[i])) ? 1 : 0;
		return s;
	}

	template <typename P> [[nodiscard]] auto rel(P pred) const -> Rel
	{
		Rel r(size());
		for (auto i = 0U; i < size(); i++)
			for (auto j = 0U; j < size(); j++)
				if (pred(i, j))
					r.set(i, j);
		return r;
	}

	[[nodiscard]] auto po() const -> Rel
	{
		Rel r(size());
		for (auto i = 0U; i < size(); i++)
			r.setRange(i, i + 1, threadEnd[i]);
		return r;
	}
	[[nodiscard]] auto sameThread() const -> Rel
	{
		Rel r(size());
		for (auto i = 0U; i < size(); i++)
			r.setRange(i, threadBegin[i], threadEnd[i]);
		return r;
	}
	[[nodiscard]] auto otherThread() const -> Rel
	{
		Rel r(size());
		for (auto i = 0U; i < size(); i++) {
			r.setRange(i, 0, threadBegin[i]);
			r.setRange(i, threadEnd[i], size());
		}
		return r;
	}
	[[nodiscard]] auto loc() const -> Rel
	{
		std::unordered_map<SAddr, std::vector<size_t>> byAddr;
		for (auto i = 0U; i < size(); i++)
			if (const auto *a = genmc::dyn_cast_if_present<MemAccessLabel>(labs[i]))
				byAddr[a->getAddr()].push_back(i);
		Rel r(size());
		for (const auto &[addr, is] : byAddr)
			for (auto i : is)
				for (auto j : is)
					r.set(i, j);
		return r;
	}

	/* The index of LAB, or -1 for the initial label and events not in G */
	[[nodiscard]] auto idx(const EventLabel *lab) const -> long
	{
		auto it = lab ? index.find(lab) : index.end();
		return it == index.end() ? -1 : static_cast<long>(it->second);
	}
	[[nodiscard]] auto idx(Event e) const -> long
	{
		return idx(g.getEventLabelIfPresent(e));
	}

	/* The relation of the dependencies of kind DEPS (a member of EventDeps) */
	template <typename D> [[nodiscard]] auto deps(D deps) const -> Rel
	{
		Rel r(size());
		for (auto j = 0U; j < size(); j++)
			if (labs[j])
				for (auto e : labs[j]->getDeps().*deps)
					if (auto i = idx(e); i >= 0)
						r.set(i, j);
		return r;
	}

	[[nodiscard]] auto rf() const -> Rel
	{
		Rel r(size());
		for (auto j = 0U; j < size(); j++)
			if (const auto *rLab = genmc::dyn_cast_if_present<ReadLabel>(labs[j]))
				if (auto i = idx(rLab->getRf()); i >= 0)
					r.set(i, j);
		return r;
	}

	[[nodiscard]] auto co() const -> Rel
	{
		Rel r(size());
		/* (A store is checked before it is placed in coherence: an error
		 * it causes is reported, and the graph checked, without its
		 * position, #202) */
		for (auto i = 0U; i < size(); i++)
			if (const auto *wLab = genmc::dyn_cast_if_present<WriteLabel>(labs[i]);
			    wLab && wLab->isInCo())
				for (const auto &sLab : g.co_succs(wLab))
					if (auto j = idx(&sLab); j >= 0)
						r.set(i, j);
		return r;
	}

	/* fr = rf^-1 ; co, with the reads of the initial value before every write */
	[[nodiscard]] auto fr(const Rel &co) const -> Rel
	{
		Rel r(size());
		for (auto i = 0U; i < size(); i++) {
			const auto *rLab = genmc::dyn_cast_if_present<ReadLabel>(labs[i]);
			if (!rLab || !rLab->getRf())
				continue;
			if (auto w = idx(rLab->getRf()); w >= 0) {
				for (auto j = 0U; j < size(); j++)
					if (co.get(w, j))
						r.set(i, j);
			} else if (genmc::isa<InitLabel>(rLab->getRf())) {
				for (auto j = 0U; j < size(); j++)
					if (const auto *wLab = genmc::dyn_cast_if_present<WriteLabel>(labs[j]);
					    wLab && wLab->getAddr() == rLab->getAddr())
						r.set(i, j);
			}
		}
		return r;
	}

	[[nodiscard]] auto rmw() const -> Rel
	{
		Rel r(size());
		for (auto i = 0U; i < size(); i++) {
			const auto *rLab = genmc::dyn_cast_if_present<ReadLabel>(labs[i]);
			if (!rLab || !rLab->isRMW())
				continue;
			const auto *wLab = genmc::dyn_cast_if_present<WriteLabel>(g.po_imm_succ(rLab));
			if (auto j = idx(wLab); wLab && wLab->isRMW() && j >= 0)
				r.set(i, j);
		}
		return r;
	}

	[[nodiscard]] auto tc() const -> Rel
	{
		Rel r(size());
		for (auto i = 0U; i < size(); i++)
			if (auto j = labs[i] ? idx(g.tc_succ(labs[i])) : -1; j >= 0)
				r.set(i, j);
		return r;
	}

	[[nodiscard]] auto tj() const -> Rel
	{
		Rel r(size());
		for (auto i = 0U; i < size(); i++)
			if (auto j = labs[i] ? idx(g.tj_succ(labs[i])) : -1; j >= 0)
				r.set(i, j);
		return r;
	}

	/* rcu-rscs */
	[[nodiscard]] auto rcuRscs() const -> Rel
	{
		Rel r(size());
		for (auto [l, u] : rscs)
			r.set(l, u);
		return r;
	}

	/* srcu-rscs */
	[[nodiscard]] auto srcuRscs() const -> Rel
	{
		Rel r(size());
		for (auto [l, u] : srcuRscsPairs)
			r.set(l, u);
		return r;
	}

	/* loc on SRCU's events (the virtual unlocks included) */
	[[nodiscard]] auto srcuLoc() const -> Rel
	{
		Rel r(size());
		for (auto i = 0U; i < size(); i++)
			for (auto j = 0U; j < size(); j++)
				if (srcuLocs[i] && srcuLocs[i] == srcuLocs[j])
					r.set(i, j);
		return r;
	}

	[[nodiscard]] auto label(size_t i) const -> const EventLabel * { return labs[i]; }

	const ExecutionGraph &g;

private:
	void addNode(const EventLabel *lab, std::optional<SAddr> srcu = std::nullopt)
	{
		if (lab)
			index[lab] = labs.size();
		labs.push_back(lab);
		srcuLocs.push_back(srcu);
	}

	/* The events the model relates: accesses, fences and RCU events, the
	 * thread events (tc, tj, thread-po), and allocations and frees (which
	 * isRcuOrderedBefore() orders) */
	static auto isNode(const EventLabel *lab) -> bool
	{
		return lab && !genmc::isa<InitLabel>(lab) &&
		       (genmc::isa<MemLabel>(lab) || genmc::isa<FenceLabel>(lab) ||
			genmc::isa<ThreadCreateLabel>(lab) || genmc::isa<ThreadStartLabel>(lab) ||
			genmc::isa<ThreadFinishLabel>(lab) || genmc::isa<ThreadJoinLabel>(lab));
	}

	/* rb relates marked events, by relations whose paths have only marked
	 * events inside, but for dependencies: a plain access matters only as
	 * a step of carry-dep (a write read by a read of its own thread, or
	 * that read) or as the plain write of addr ; [Plain] ; wmb. Allocations
	 * and frees matter to none */
	auto mattersToRb(const EventLabel *lab) const -> bool
	{
		if (!lab->isNotAtomic())
			return true;
		if (const auto *rLab = genmc::dyn_cast<ReadLabel>(lab))
			return rLab->getRf() && rLab->readsInt() &&
			       !genmc::isa<InitLabel>(rLab->getRf());
		if (const auto *wLab = genmc::dyn_cast<WriteLabel>(lab)) {
			if (auto rs = g.rfi_succs(wLab); rs.begin() != rs.end())
				return true;
			for (const auto &other : g.po_succs(wLab))
				for (auto e : other.addr())
					if (e == wLab->getPos())
						return true;
			return false;
		}
		return !genmc::isa<MemLabel>(lab);
	}

	std::vector<const EventLabel *> labs;
	std::vector<size_t> threadBegin;
	std::vector<size_t> threadEnd;
	std::unordered_map<const EventLabel *, size_t> index;
	std::vector<std::pair<size_t, size_t>> rscs;
	std::vector<std::pair<size_t, size_t>> srcuRscsPairs;
	std::vector<std::optional<SAddr>> srcuLocs;
};

/* What orders two conflicting accesses, for the plain-access rules, and
 * GenMC's thread order (po | tc | tj)+ */
struct PlainRules {
	Rel wwVis, wrVis, rwXbstar, threadOrder;
};

/* The model's relations on a graph */
struct Relations {
	/* Unless FULL, the verdict's pb is not decided */
	Relations(const Model &m, bool full);

	[[nodiscard]] auto rcuEvents() const -> Set;
	[[nodiscard]] auto reachFrom(const Set &f) const -> Rel;
	[[nodiscard]] auto rcuOrder(const Rel &reach) const -> Rel;
	/* rcu-fence */
	[[nodiscard]] auto rcuFence() const -> Rel;
	/* The plain-access rules, with RCU's rb when HASGP */
	[[nodiscard]] auto plainRules(bool hasGp) const -> PlainRules;

	const Model &m;
	Set marked, plain, access, W, r4rmb, noreturn, release, threadOut, threadIn;
	Rel po, poq, rf, co, fr, rmw, tc, tj;
	Rel in, ext, rfe, carryAddr, poRel, acqPo, rmbRel, wmb, ulp, strongFence, fence,
		cumulFence, rmwSequence;
	Rel prop, hb, hbStar, pb;
	lkmm::Verdict verdict;
};

Relations::Relations(const Model &mod, bool full)
	: m(mod), po(m.po()), poq(po.opt()), rf(m.rf()), co(m.co()), fr(m.fr(co)), rmw(m.rmw()),
	  tc(m.tc()), tj(m.tj()), in(0), ext(0), rfe(0), carryAddr(0), poRel(0), acqPo(0),
	  rmbRel(0), wmb(0), ulp(0), strongFence(0), fence(0), cumulFence(0), rmwSequence(0),
	  prop(0), hb(0), hbStar(0), pb(0)
{
	/* Events */
	auto R = m.set([](auto *l) { return genmc::isa<ReadLabel>(l); });
	W = m.set([](auto *l) { return genmc::isa<WriteLabel>(l); });
	auto M = m.set([](auto *l) { return genmc::isa<MemAccessLabel>(l); });
	access = M;
	marked = m.set([](auto *l) { return !l->isNotAtomic(); });
	plain = m.set([](auto *l) { return l->isNotAtomic(); });
	auto kind = [&](LkmmFenceKind k) {
		return m.set([k](auto *l) { return isFence(l, k); });
	};
	auto mbR = m.set([](auto *l) { return genmc::isa<ReadLabel>(l) && l->isLkmmMb(); });
	auto mbW = m.set([](auto *l) { return genmc::isa<WriteLabel>(l) && l->isLkmmMb(); });
	r4rmb = m.set([](auto *l) { return genmc::isa<ReadLabel>(l) && !l->isLkmmNoreturn(); });
	noreturn = m.set([](auto *l) { return genmc::isa<ReadLabel>(l) && l->isLkmmNoreturn(); });
	/* The bell's Acquire and Release: no fences, no failed RMWs */
	auto acq = m.set([](auto *l) {
		const auto *r = genmc::dyn_cast<ReadLabel>(l);
		return r && r->isAtLeastAcquire() && (!genmc::isa<CasReadLabel>(r) || r->isRMW());
	});
	release = m.set([](auto *l) { return genmc::isa<WriteLabel>(l) && l->isAtLeastRelease(); });
	auto lkr = m.set([](auto *l) {
		return (genmc::isa<LockCasReadLabel>(l) || genmc::isa<TrylockCasReadLabel>(l)) &&
		       genmc::cast<ReadLabel>(l)->isRMW();
	});
	auto lkw = m.set([](auto *l) {
		return genmc::isa<LockCasWriteLabel>(l) || genmc::isa<TrylockCasWriteLabel>(l);
	});
	auto ul = m.set([](auto *l) { return genmc::isa<UnlockWriteLabel>(l); });
	auto excl = m.set(isRMWAccess);
	threadOut = m.set([](auto *l) {
		return genmc::isa<ThreadCreateLabel>(l) || genmc::isa<ThreadFinishLabel>(l);
	});
	threadIn = m.set([](auto *l) {
		return genmc::isa<ThreadStartLabel>(l) || genmc::isa<ThreadJoinLabel>(l);
	});
	auto syncRcu = kind(LkmmFenceKind::SyncRcu);
	auto syncSrcu = kind(LkmmFenceKind::SyncSrcu);
	auto srcuUnlock = m.set([](auto *l) { return genmc::isa<SrcuUnlockWriteLabel>(l); });
	auto notSrcuUnlock = srcuUnlock;
	for (auto &b : notSrcuUnlock)
		b = !b;

	/* Base relations */
	in = m.sameThread();
	ext = m.otherThread();
	auto addr = m.deps(&EventDeps::addr);
	auto data = m.deps(&EventDeps::data);
	auto ctrl = m.deps(&EventDeps::ctrl);
	rfe = rf & ext;
	auto rfi = rf & in;
	auto coe = co & ext;
	auto fre = fr & ext;
	auto overwrite = co | fr;

	/* Fences */
	auto fencerel = [&](const Set &f) { return po.range(f).seq(po); };
	auto fenceAt = [&](LkmmFenceKind k) { return fencerel(kind(k)); };
	acqPo = po.restrict(acq, M);
	poRel = po.restrict(M, release);
	auto ulpInt = po.range(ul).seq(po | rfi).range(lkr).seq(po);
	auto ulpExt = po.range(ul).seq(rfe).range(lkr).seq(po);
	ulp = ulpInt | ulpExt;
	rmbRel = fenceAt(LkmmFenceKind::Rmb);
	auto rmb = rmbRel.restrict(r4rmb, r4rmb);
	wmb = fenceAt(LkmmFenceKind::Wmb).restrict(W, W);
	/* The cat's [M] endpoints, with GenMC's thread events: creation and
	 * join synchronise like a release and an acquire, which a strong fence
	 * reaches (the kat's mb-int and mb-ext) */
	auto MT = M;
	for (auto i = 0U; i < MT.size(); i++)
		MT[i] = MT[i] || threadOut[i] || threadIn[i];
	auto mb = fenceAt(LkmmFenceKind::Mb).restrict(MT, MT) | po.restrict(MT, mbR) |
		  po.restrict(mbW, MT) |
		  fenceAt(LkmmFenceKind::BeforeAtomic).range(excl).seq(poq).restrict(MT, MT) |
		  poq.range(excl).seq(fenceAt(LkmmFenceKind::AfterAtomic)).restrict(MT, MT) |
		  poq.range(lkw).seq(fenceAt(LkmmFenceKind::AfterSpinlock)).restrict(MT, MT) |
		  ulp.range(kind(LkmmFenceKind::AfterUnlockLock)).seq(po).restrict(MT, MT) |
		  poq.range(srcuUnlock)
			  .seq(fenceAt(LkmmFenceKind::AfterSrcuReadUnlock))
			  .restrict(MT, MT);
	auto syncAny = syncRcu;
	for (auto i = 0U; i < syncAny.size(); i++)
		syncAny[i] = syncAny[i] || syncSrcu[i];
	auto gp = po.range(syncAny).seq(poq);
	strongFence = mb | gp;
	fence = strongFence | poRel | acqPo | wmb | rmb;

	/* Preserved program order (GenMC's dependencies: rmw? after them) */
	auto rmwq = rmw.opt();
	auto carryDep = data.seq(rmwq).range(notSrcuUnlock).seq(rfi).star();
	auto dep = carryDep.seq(addr | data);
	auto cdep = carryDep.seq(ctrl);
	auto rwdep = (dep | cdep).seq(rmwq).range(W);
	carryAddr = carryDep.seq(addr);
	auto toW = rwdep | (overwrite & in) | carryAddr.range(plain).seq(wmb);
	auto toR = carryAddr.range(R) | dep.seq(rmwq).range(marked).seq(rfi);
	auto ppo = toR | toW | (fence & in) | (ulp & in);
	auto threadPo = po.range(threadOut) | po.dom(threadIn);

	/* Coherence and atomicity */
	verdict.coherence = ((po & m.loc()) | rf | co | fr).acyclic();
	verdict.atomicity = (rmw & fre.seq(coe)).empty();

	/* Propagation and happens-before */
	rmwSequence = rf.seq(rmw).star();
	auto aCumul = rfe.range(marked).opt().seq(strongFence | poRel);
	cumulFence = (aCumul | wmb | ulp).restrict(marked, marked).seq(rmwSequence);
	prop = ((overwrite & ext).opt().seq(cumulFence.star()).range(marked).seq(rfe.opt()))
		       .restrict(marked, marked);
	auto propInt = (prop - Rel::id(Set(m.size(), 1))) & in;
	hb = (ppo | threadPo | rfe | propInt | tc | tj).restrict(marked, marked);
	auto hbPlus = hb.plus();
	verdict.hb = hbPlus.irreflexive();
	hbStar = hbPlus.opt();
	pb = prop.seq(strongFence).seq(hbStar).range(marked);
	if (full)
		verdict.pb = pb.acyclic();
}

/* RCU's events: grace periods and the ends of critical sections (the
 * virtual unlocks included); rcu-order relates them only */
auto Relations::rcuEvents() const -> Set
{
	auto f = m.set([](auto *l) {
		return isFence(l, LkmmFenceKind::SyncRcu) || isFence(l, LkmmFenceKind::SyncSrcu);
	});
	m.rcuRscs().forAll([&](size_t l, size_t u) { f[l] = f[u] = 1; });
	m.srcuRscs().forAll([&](size_t l, size_t u) { f[l] = f[u] = 1; });
	return f;
}

/* po? ; hb* ; pb* from the events of F (only their rows) */
auto Relations::reachFrom(const Set &f) const -> Rel
{
	/* pb* on these rows only: the closure of the whole of pb is not needed */
	auto reach = poq.dom(f).seq(hbStar);
	for (;;) {
		auto next = reach | reach.seq(pb);
		if (next == reach)
			return reach;
		reach = std::move(next);
	}
}

/* rcu-order, from REACH = reachFrom(rcuEvents()) */
auto Relations::rcuOrder(const Rel &reach) const -> Rel
{
	auto rcuGp = Rel::id(m.set([](auto *l) { return isFence(l, LkmmFenceKind::SyncRcu); }));
	auto srcuGp = Rel::id(m.set([](auto *l) { return isFence(l, LkmmFenceKind::SyncSrcu); }));
	auto rcuRscsi = m.rcuRscs().inverse();
	auto srcuRscsi = m.srcuRscs().inverse();
	auto loc = m.srcuLoc();
	/* rcu-link, for the rows of RCU's events (the only ones rcu-order uses) */
	auto link = reach.seq(prop).seq(po);
	auto gpLink = rcuGp.seq(link);
	auto csLink = rcuRscsi.seq(link);
	auto sgpLink = srcuGp.seq(link);
	auto scsLink = srcuRscsi.seq(link);
	auto order = rcuGp | srcuGp | gpLink.seq(rcuRscsi) | csLink.seq(rcuGp) |
		     (sgpLink.seq(srcuRscsi) & loc) | (scsLink.seq(srcuGp) & loc);
	for (;;) {
		auto orderLink = order.seq(link);
		auto next = order | gpLink.seq(orderLink).seq(rcuRscsi) |
			    csLink.seq(orderLink).seq(rcuGp) |
			    (sgpLink.seq(orderLink).seq(srcuRscsi) & loc) |
			    (scsLink.seq(orderLink).seq(srcuGp) & loc) | orderLink.seq(order);
		if (next == order)
			break;
		order = std::move(next);
	}
	return order;
}

/* rcu-fence = po ; rcu-order ; po? */
auto Relations::rcuFence() const -> Rel
{
	auto f = rcuEvents();
	return po.seq(rcuOrder(reachFrom(f))).seq(poq);
}

/* The cat's plain-access part. Its fence and strong-fence gain rcu-fence
 * (nonrw-fence, bound before, does not). GenMC's thread creation and join
 * synchronise like a release write read by an acquire read: the thread
 * events are releases (TC, TE) and acquires (TB, TJ) to po-rel, acq-po and
 * the fences, and tc and tj are rfe, in cumul-fence too (as a release is
 * A-cumulative), as the kat's thread-po makes them for hb */
auto Relations::plainRules(bool hasGp) const -> PlainRules
{
	const auto n = m.size();
	auto rcuF = hasGp ? rcuFence() : Rel(n);
	auto rb = hasGp ? prop.seq(rcuF).seq(hbStar).seq(pb.star()).range(marked) : Rel(n);
	auto xbstar = (hb | pb | rb).star();

	auto MT = access;
	auto relT = release;
	for (auto i = 0U; i < n; i++) {
		MT[i] = MT[i] || threadOut[i] || threadIn[i];
		relT[i] = relT[i] || threadOut[i];
	}
	auto threadRel = po.range(threadOut) | po.dom(threadIn);
	auto rfeT = rfe | tc | tj;
	auto poRelT = po.restrict(MT, relT);
	auto sf = strongFence | rcuF;
	auto fen = fence | rcuF | threadRel;
	auto nonrwFence = strongFence | poRel | acqPo | threadRel;
	/* cumul-fence is bound before strong-fence gains rcu-fence */
	auto aCumul = rfeT.range(marked).opt().seq(strongFence | poRelT);
	auto cumulFenceT = (aCumul | wmb | ulp).restrict(marked, marked).seq(rmwSequence);
	auto vis = cumulFenceT.star()
			   .seq(rfeT.opt())
			   .range(marked)
			   .seq(sf.range(marked).seq(xbstar) | (xbstar & in));

	/* Boundaries for the lifetimes of plain accesses */
	auto notNoreturn = noreturn;
	for (auto &b : notNoreturn)
		b = !b;
	auto wPre = (carryAddr | fen).opt().dom(marked);
	auto rPre = (carryAddr | nonrwFence | rmbRel.restrict(r4rmb, notNoreturn)).opt().dom(marked);
	auto wPost = fen.opt().range(marked).seq(rmwSequence);
	auto rPost = (nonrwFence | rmbRel.restrict(notNoreturn, r4rmb)).opt().range(marked);

	return {.wwVis = fen | sf.seq(xbstar).seq(wPre) | wPost.seq(vis).seq(wPre),
		.wrVis = fen | sf.seq(xbstar).seq(rPre) | wPost.seq(vis).seq(rPre),
		.rwXbstar = fen | rPost.seq(xbstar).seq(wPre),
		.threadOrder = (po | tc | tj).plus()};
}

/* Whether accesses I and J are a potential race (the cat's pre-race): of
 * different threads, to the same location, at least one plain (the initial
 * writes are none). GenMC leaves out the pairs that thread creation and
 * join order: these are full synchronisation, and GenMC's coherence
 * already orders such a pair by them */
auto isPreRace(const Relations &r, const PlainRules &rules, size_t i, size_t j) -> bool
{
	return r.access[i] && r.access[j] && (r.plain[i] || r.plain[j]) && r.ext.get(i, j) &&
	       !rules.threadOrder.get(i, j) && !rules.threadOrder.get(j, i);
}

/* The pairs of a potential race: a write and a read (rf, or co? ; rf for
 * the races), a read and a write (fr), two writes (co) */
enum class Pair { WR, RW, WW };

/* Calls F(pair, a, b) on each potential race (a, b) */
template <typename F>
void forEachPreRace(const Relations &r, const PlainRules &rules, bool coRf, F f)
{
	auto visit = [&](Pair pair, const Rel &rel) {
		rel.forAll([&](size_t i, size_t j) {
			if (isPreRace(r, rules, i, j))
				f(pair, i, j);
		});
	};
	visit(Pair::WR, coRf ? r.co.opt().seq(r.rf) : r.rf);
	visit(Pair::RW, r.fr);
	visit(Pair::WW, r.co);
}

/* GenMC's thread order (po | tc | tj)+ on a graph, by the clocks of its
 * threads: a thread's clock changes at its start and at its joins */
class ThreadOrder {
public:
	explicit ThreadOrder(const ExecutionGraph &g) : g(g), segments(g.getNumThreads()) {}

	/* Whether A is before B */
	auto before(const EventLabel *a, const EventLabel *b) -> bool
	{
		if (a->getThread() == b->getThread())
			return a->getIndex() < b->getIndex();
		return a->getIndex() <= clockAt(b->getThread(), b->getIndex())[a->getThread()];
	}

private:
	/* The last index of each thread before an event (-1 for none) */
	using Clock = std::vector<int>;

	auto clockAt(int t, int index) -> Clock
	{
		auto &segs = segments[t];
		if (segs.empty())
			computeSegments(t);
		const Clock *c = &segs.front().second;
		for (const auto &[from, clock] : segs)
			if (from <= index)
				c = &clock;
		auto result = *c;
		result[t] = index;
		return result;
	}

	void computeSegments(int t)
	{
		auto &segs = segments[t];
		Clock start(g.getNumThreads(), -1);
		const auto *bLab = genmc::dyn_cast<ThreadStartLabel>(g.getFirstThreadLabel(t));
		if (const auto *cLab = bLab ? bLab->getCreate() : nullptr)
			start = clockAt(cLab->getThread(), cLab->getIndex());
		segs.emplace_back(0, start);
		for (auto k = 1; k < g.getThreadSize(t); k++) {
			const auto *jLab =
				genmc::dyn_cast_if_present<ThreadJoinLabel>(g.getEventLabelIfPresent(Event(t, k)));
			if (!jLab || g.getLastThreadLabel(jLab->getChildId()) == nullptr)
				continue;
			const auto *last = g.getLastThreadLabel(jLab->getChildId());
			auto child = clockAt(last->getThread(), last->getIndex());
			auto merged = segments[t].back().second;
			for (auto u = 0U; u < merged.size(); u++)
				merged[u] = std::max(merged[u], child[u]);
			segments[t].emplace_back(k, std::move(merged));
		}
	}

	const ExecutionGraph &g;
	std::vector<std::vector<std::pair<int, Clock>>> segments;
};

} // namespace

auto lkmm::hasPreRace(const ExecutionGraph &g) -> bool
{
	/* The accesses of each location that another thread's access to it
	 * shares (as plain accesses are usually few, by their locations) */
	std::unordered_map<SAddr, std::vector<const MemAccessLabel *>> byAddr;
	auto plain = false;
	for (const auto &lab : g.labels())
		if (const auto *a = genmc::dyn_cast<MemAccessLabel>(&lab)) {
			byAddr[a->getAddr()].push_back(a);
			plain |= a->isNotAtomic();
		}
	if (!plain)
		return false;

	ThreadOrder order(g);
	for (const auto &[addr, accesses] : byAddr)
		for (auto i = 0U; i < accesses.size(); i++)
			for (auto j = i + 1; j < accesses.size(); j++) {
				const auto *a = accesses[i];
				const auto *b = accesses[j];
				if (a->getThread() != b->getThread() &&
				    (a->isNotAtomic() || b->isNotAtomic()) && !order.before(a, b) &&
				    !order.before(b, a))
					return true;
			}
	return false;
}

auto lkmm::hasThreadBlockedBeforeKept(const ExecutionGraph &g) -> bool
{
	return std::ranges::any_of(g.thr_ids(), [&](auto tid) {
		const auto *bLab = genmc::dyn_cast_if_present<BlockLabel>(g.getLastThreadLabel(tid));
		return bLab && bLab->isAfterKept();
	});
}

auto lkmm::hasGracePeriod(const ExecutionGraph &g) -> bool
{
	for (const auto &lab : g.labels())
		if (isFence(&lab, LkmmFenceKind::SyncRcu) || isFence(&lab, LkmmFenceKind::SyncSrcu))
			return true;
	return false;
}

auto lkmm::evaluate(const ExecutionGraph &g, bool full, bool closeOpen) -> Verdict
{
	auto plain = hasPreRace(g);
	Model m(g, closeOpen, !full && !plain);
	Relations r(m, full);
	auto v = r.verdict;
	if (!v.hb || !v.pb)
		return v;
	if (plain) {
		auto rules = r.plainRules(hasGracePeriod(g));
		/* wr-incoh = pre-race & rf & rw-xbstar^-1, rw-incoh = pre-race & fr &
		 * wr-vis^-1, ww-incoh = pre-race & co & ww-vis^-1 */
		forEachPreRace(r, rules, false, [&](Pair pair, size_t a, size_t b) {
			const auto &back = pair == Pair::WR   ? rules.rwXbstar
					   : pair == Pair::RW ? rules.wrVis
							      : rules.wwVis;
			if (back.get(b, a))
				v.plainCoherence = false;
		});
	}
	if (!hasGracePeriod(g))
		return v;

	/* RCU: rb = prop ; po ; rcu-order ; po? ; hb* ; pb* ; [Marked] is
	 * reflexive iff some marked i reaches an RCU event a by prop ; po, and
	 * comes back from b with rcu-order(a, b) by po? ; hb* ; pb* */
	auto f = r.rcuEvents();
	auto reach = r.reachFrom(f);
	auto order = r.rcuOrder(reach);
	auto propPo = r.prop.seq(r.po.range(f));
	for (auto i = 0U; i < m.size() && v.rcu; i++) {
		if (!r.marked[i])
			continue;
		propPo.forEachSucc(i, [&](size_t a) {
			order.forEachSucc(a, [&](size_t b) {
				if (reach.get(b, i))
					v.rcu = false;
			});
		});
	}
	return v;
}

auto lkmm::findRace(const ExecutionGraph &g) -> std::optional<Race>
{
	if (!hasPreRace(g))
		return std::nullopt;
	Model m(g, false);
	Relations r(m, true);
	auto rules = r.plainRules(hasGracePeriod(g));
	std::optional<Race> race;
	forEachPreRace(r, rules, true, [&](Pair pair, size_t a, size_t b) {
		if (race)
			return;
		/* ww-race = (pre-race & co) \ ww-nonrace, with ww-nonrace = ww-vis &
		 * ((Marked * W) | rw-xbstar) & ((W * Marked) | wr-vis);
		 * wr-race = (pre-race & (co? ; rf)) \ wr-vis \ rw-xbstar^-1;
		 * rw-race = (pre-race & fr) \ rw-xbstar */
		auto racy = false;
		switch (pair) {
		case Pair::WW:
			racy = !(rules.wwVis.get(a, b) && (r.marked[a] || rules.rwXbstar.get(a, b)) &&
				 (r.marked[b] || rules.wrVis.get(a, b)));
			break;
		case Pair::WR:
			racy = !rules.wrVis.get(a, b) && !rules.rwXbstar.get(b, a);
			break;
		case Pair::RW:
			racy = !rules.rwXbstar.get(a, b);
			break;
		}
		static constexpr const char *names[] = {"wr-race", "rw-race", "ww-race"};
		if (racy)
			race = Race{.a = m.label(a), .b = m.label(b),
				    .kind = names[static_cast<int>(pair)]};
	});
	return race;
}

auto lkmm::findUnorderedFree(const ExecutionGraph &g,
			     const std::function<bool(const EventLabel *, const EventLabel *)> &isHb)
	-> std::optional<std::pair<const EventLabel *, const EventLabel *>>
{
	std::optional<Rel> order;
	std::optional<Model> m;
	for (const auto &lab : g.labels()) {
		const auto *a = genmc::dyn_cast<MemAccessLabel>(&lab);
		auto fPos = a ? g.free_pos(a) : std::nullopt;
		if (!fPos)
			continue;
		const auto *f = g.getEventLabel(*fPos);
		if (genmc::isa<HpRetireLabel>(f) || isHb(a, f))
			continue;
		if (!order) {
			m.emplace(g, false);
			Relations r(*m, false);
			order = (r.po | r.rf | r.tc | r.tj | r.rcuFence()).plus();
		}
		auto i = m->idx(a);
		auto j = m->idx(f);
		if (i < 0 || j < 0 || !order->get(i, j))
			return std::make_pair(a, f);
	}
	return std::nullopt;
}

auto lkmm::isRcuOrderedBefore(const ExecutionGraph &g, const EventLabel *a, const EventLabel *b,
			      bool closeOpen) -> bool
{
	Model m(g, closeOpen);
	Relations r(m, false);
	auto order = (r.po | r.rf | r.tc | r.tj | r.rcuFence()).plus();
	auto i = m.idx(a);
	auto j = m.idx(b);
	return i >= 0 && j >= 0 && order.get(i, j);
}
