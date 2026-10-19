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
 * linux-kernel.cat, marked accesses) evaluated on a whole execution graph.
 * The events are the graph's labels but the initial one (nothing precedes
 * it, so it lies on no cycle); Marked is GenMC's atomic labels, fences and
 * RCU events included, as in herd7. GenMC's additions are those of
 * kat/lkmm-genmc.kat in the Kater fork: dependencies reach the write of an
 * RMW (rmw?), and thread creation and join order (thread-po, tc, tj).
 *
 * RCU, as the cat file has it:
 *
 *   rcu-rscs: the nested rcu_read_lock()/rcu_read_unlock() pairs of a thread
 *   rcu-link = po? ; hb* ; pb* ; prop ; po
 *   rcu-order = the least fixpoint of
 *       rcu-gp | (rcu-gp ; rcu-link ; rcu-rscsi) | (rcu-rscsi ; rcu-link ; rcu-gp)
 *     | (rcu-gp ; rcu-link ; rcu-order ; rcu-link ; rcu-rscsi)
 *     | (rcu-rscsi ; rcu-link ; rcu-order ; rcu-link ; rcu-gp)
 *     | (rcu-order ; rcu-link ; rcu-order)
 *   rcu-fence = po ; rcu-order ; po?
 *   rb = prop ; rcu-fence ; hb* ; pb* ; [Marked]          irreflexive rb
 *
 * fence and strong-fence also gain rcu-fence after rb, but only the
 * plain-access rules use those.
 */

#include "genmc/Execution/LkmmModel.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Support/Cast.hpp"

#include <cstdint>
#include <limits>
#include <optional>
#include <unordered_map>
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

/* The events and base relations of a graph. With CLOSEOPEN, each
 * rcu_read_lock() still unmatched gets a virtual rcu_read_unlock() at the
 * end of its thread (a node without a label) */
class Model {
public:
	/* With PRUNE, the nodes are only those that can matter to rb (see
	 * mattersToRb()); coherence and the freed-memory ordering need all */
	Model(const ExecutionGraph &g, bool closeOpen, bool prune = false) : g(g)
	{
		/* By thread, in po order: a thread's events are [begin, end) */
		for (auto t = 0U; t < g.getNumThreads(); t++) {
			auto begin = labs.size();
			std::vector<size_t> open;
			for (auto k = 0; k < g.getThreadSize(t); k++) {
				const auto *lab = g.getEventLabelIfPresent(Event(t, k));
				if (!isNode(lab) || (prune && !mattersToRb(lab)))
					continue;
				addNode(lab);
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
			for (auto i = begin; i < labs.size(); i++)
				threadEnd.push_back(labs.size());
			for (auto i = begin; i < labs.size(); i++)
				threadBegin.push_back(begin);
		}
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
		for (auto i = 0U; i < size(); i++)
			if (genmc::isa_and_present<WriteLabel>(labs[i]))
				for (const auto &sLab : g.co_succs(labs[i]))
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

	const ExecutionGraph &g;

private:
	void addNode(const EventLabel *lab)
	{
		if (lab)
			index[lab] = labs.size();
		labs.push_back(lab);
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

	const Model &m;
	Set marked;
	Rel po, poq, rf, co, fr, rmw, tc, tj;
	Rel prop, hb, hbStar, pb;
	lkmm::Verdict verdict;
};

Relations::Relations(const Model &mod, bool full)
	: m(mod), po(m.po()), poq(po.opt()), rf(m.rf()), co(m.co()), fr(m.fr(co)), rmw(m.rmw()),
	  tc(m.tc()), tj(m.tj()), prop(0), hb(0), hbStar(0), pb(0)
{
	/* Events */
	auto R = m.set([](auto *l) { return genmc::isa<ReadLabel>(l); });
	auto W = m.set([](auto *l) { return genmc::isa<WriteLabel>(l); });
	auto M = m.set([](auto *l) { return genmc::isa<MemAccessLabel>(l); });
	marked = m.set([](auto *l) { return !l->isNotAtomic(); });
	auto plain = m.set([](auto *l) { return l->isNotAtomic(); });
	auto kind = [&](LkmmFenceKind k) {
		return m.set([k](auto *l) { return isFence(l, k); });
	};
	auto mbR = m.set([](auto *l) { return genmc::isa<ReadLabel>(l) && l->isLkmmMb(); });
	auto mbW = m.set([](auto *l) { return genmc::isa<WriteLabel>(l) && l->isLkmmMb(); });
	auto r4rmb = m.set([](auto *l) { return genmc::isa<ReadLabel>(l) && !l->isLkmmNoreturn(); });
	/* The bell's Acquire and Release: no fences, no failed RMWs */
	auto acq = m.set([](auto *l) {
		const auto *r = genmc::dyn_cast<ReadLabel>(l);
		return r && r->isAtLeastAcquire() && (!genmc::isa<CasReadLabel>(r) || r->isRMW());
	});
	auto rel = m.set([](auto *l) { return genmc::isa<WriteLabel>(l) && l->isAtLeastRelease(); });
	auto lkr = m.set([](auto *l) {
		return (genmc::isa<LockCasReadLabel>(l) || genmc::isa<TrylockCasReadLabel>(l)) &&
		       genmc::cast<ReadLabel>(l)->isRMW();
	});
	auto lkw = m.set([](auto *l) {
		return genmc::isa<LockCasWriteLabel>(l) || genmc::isa<TrylockCasWriteLabel>(l);
	});
	auto ul = m.set([](auto *l) { return genmc::isa<UnlockWriteLabel>(l); });
	auto excl = m.set(isRMWAccess);
	auto threadOut = m.set([](auto *l) {
		return genmc::isa<ThreadCreateLabel>(l) || genmc::isa<ThreadFinishLabel>(l);
	});
	auto threadIn = m.set([](auto *l) {
		return genmc::isa<ThreadStartLabel>(l) || genmc::isa<ThreadJoinLabel>(l);
	});
	auto syncRcu = kind(LkmmFenceKind::SyncRcu);

	/* Base relations */
	auto in = m.sameThread();
	auto ext = m.otherThread();
	auto addr = m.deps(&EventDeps::addr);
	auto data = m.deps(&EventDeps::data);
	auto ctrl = m.deps(&EventDeps::ctrl);
	auto rfe = rf & ext;
	auto rfi = rf & in;
	auto coe = co & ext;
	auto fre = fr & ext;
	auto overwrite = co | fr;

	/* Fences */
	auto fencerel = [&](const Set &f) { return po.range(f).seq(po); };
	auto fenceAt = [&](LkmmFenceKind k) { return fencerel(kind(k)); };
	auto acqPo = po.restrict(acq, M);
	auto poRel = po.restrict(M, rel);
	auto ulpInt = po.range(ul).seq(po | rfi).range(lkr).seq(po);
	auto ulpExt = po.range(ul).seq(rfe).range(lkr).seq(po);
	auto ulp = ulpInt | ulpExt;
	auto rmb = fenceAt(LkmmFenceKind::Rmb).restrict(r4rmb, r4rmb);
	auto wmb = fenceAt(LkmmFenceKind::Wmb).restrict(W, W);
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
		  ulp.range(kind(LkmmFenceKind::AfterUnlockLock)).seq(po).restrict(MT, MT);
	auto gp = po.range(syncRcu).seq(poq);
	auto strongFence = mb | gp;
	auto fence = strongFence | poRel | acqPo | wmb | rmb;

	/* Preserved program order (GenMC's dependencies: rmw? after them) */
	auto rmwq = rmw.opt();
	auto carryDep = data.seq(rmwq).seq(rfi).star();
	auto dep = carryDep.seq(addr | data);
	auto cdep = carryDep.seq(ctrl);
	auto rwdep = (dep | cdep).seq(rmwq).range(W);
	auto toW = rwdep | (overwrite & in) | carryDep.seq(addr).range(plain).seq(wmb);
	auto toR = carryDep.seq(addr).range(R) | dep.seq(rmwq).range(marked).seq(rfi);
	auto ppo = toR | toW | (fence & in) | (ulp & in);
	auto threadPo = po.range(threadOut) | po.dom(threadIn);

	/* Coherence and atomicity */
	verdict.coherence = ((po & m.loc()) | rf | co | fr).acyclic();
	verdict.atomicity = (rmw & fre.seq(coe)).empty();

	/* Propagation and happens-before */
	auto rmwSequence = rf.seq(rmw).star();
	auto aCumul = rfe.range(marked).opt().seq(strongFence | poRel);
	auto cumulFence = (aCumul | wmb | ulp).restrict(marked, marked).seq(rmwSequence);
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
	auto f = m.set([](auto *l) { return isFence(l, LkmmFenceKind::SyncRcu); });
	m.rcuRscs().forAll([&](size_t l, size_t u) { f[l] = f[u] = 1; });
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
	auto rcuRscsi = m.rcuRscs().inverse();
	/* rcu-link, for the rows of RCU's events (the only ones rcu-order uses) */
	auto link = reach.seq(prop).seq(po);
	auto gpLink = rcuGp.seq(link);
	auto csLink = rcuRscsi.seq(link);
	auto order = rcuGp | gpLink.seq(rcuRscsi) | csLink.seq(rcuGp);
	for (;;) {
		auto orderLink = order.seq(link);
		auto next = order | gpLink.seq(orderLink).seq(rcuRscsi) |
			    csLink.seq(orderLink).seq(rcuGp) | orderLink.seq(order);
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

} // namespace

auto lkmm::hasGracePeriod(const ExecutionGraph &g) -> bool
{
	for (const auto &lab : g.labels())
		if (isFence(&lab, LkmmFenceKind::SyncRcu))
			return true;
	return false;
}

auto lkmm::evaluate(const ExecutionGraph &g, bool full) -> Verdict
{
	Model m(g, false, !full);
	Relations r(m, full);
	auto v = r.verdict;
	if (!v.hb || !v.pb || !hasGracePeriod(g))
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

auto lkmm::isRcuOrderedBefore(const ExecutionGraph &g, const EventLabel *a, const EventLabel *b)
	-> bool
{
	Model m(g, true);
	Relations r(m, false);
	auto order = (r.po | r.rf | r.tc | r.tj | r.rcuFence()).plus();
	auto i = m.idx(a);
	auto j = m.idx(b);
	return i >= 0 && j >= 0 && order.get(i, j);
}
