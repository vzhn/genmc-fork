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


/*
 * The graph operations of the exploration against the invariants of
 * -check-invariants: random sequences of additions (fences, reads, writes,
 * thread creations), removals of a thread's last label, cuts to a stamp,
 * copies up to a view, relocations of a thread's events (with holes, under
 * dependency tracking) and cached prefix views. After every operation the
 * graph satisfies I1 (invariants::checkGraph(), with the plan of a cut or a
 * copy), and every cached prefix view names only events of the graph that
 * are not holes. A failing sequence is shrunk to a minimal one.
 *
 * Each operation is drawn from the generated integers against the current
 * graph, so that every sequence is valid and shrinking keeps it so.
 * (Issue #222: a copy kept prefix views naming events it did not keep, and
 * closing slots left the maximum stamp above the newest label's; #218 and
 * the first version of its fix: caches that a cut or a copy left stale.)
 */

#include "StubChecker.hpp"
#include "genmc/ADT/DepView.hpp"
#include "genmc/ADT/View.hpp"
#include "genmc/Execution/DepExecutionGraph.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Support/ThreadInfo.hpp"
#include "genmc/Verification/Checks/Invariants.hpp"

#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <format>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace {

enum class Op { Fence, Write, Read, Create, RemoveLast, Cut, Copy, Relocate, CachePrefix, Count };

/** Draws the parameters of the operations from the generated integers */
class Draws {
public:
	explicit Draws(const std::vector<unsigned> &vals) : vals_(vals) {}

	[[nodiscard]] auto done() const -> bool { return pos_ >= vals_.size(); }

	/** A value in [0, N) (0 once the integers run out) */
	auto below(unsigned n) -> unsigned
	{
		if (n == 0 || done())
			return 0;
		return vals_[pos_++] % n;
	}

private:
	const std::vector<unsigned> &vals_;
	size_t pos_ = 0;
};

class Harness {
public:
	explicit Harness(bool depTracking) : dep_(depTracking)
	{
		auto conf = ExecutionGraph::Config{.memModel = &checker_};
		if (dep_)
			g_ = std::make_unique<DepExecutionGraph>(conf);
		else
			g_ = std::make_unique<ExecutionGraph>(conf);
	}

	/** Applies the next operation; the log names what was done */
	void step(Draws &d)
	{
		switch (static_cast<Op>(d.below(static_cast<unsigned>(Op::Count)))) {
		case Op::Fence:
			if (auto t = liveThread(d))
				add(FenceLabel::create(next(*t), MemOrdering::SequentiallyConsistent));
			break;
		case Op::Write:
			if (auto t = liveThread(d)) {
				auto addr = addrs_[d.below(addrs_.size())];
				auto *wLab = genmc::cast<WriteLabel>(add(WriteLabel::create(
					next(*t), MemOrdering::Relaxed, addr, ASize(4), SVal(1))));
				wLab->addCo(g_->co_max(addr));
			}
			break;
		case Op::Read:
			if (auto t = liveThread(d))
				read(*t, d);
			break;
		case Op::Create:
			if (auto t = liveThread(d); t && g_->getNumThreads() < kMaxThreads)
				create(*t);
			break;
		case Op::RemoveLast:
			removeLast(d);
			break;
		case Op::Cut:
			cut(d);
			break;
		case Op::Copy:
			copy(d);
			break;
		case Op::Relocate:
			if (dep_)
				relocate(d);
			break;
		case Op::CachePrefix:
			cachePrefix(d);
			break;
		case Op::Count:
			break;
		}
		check(nullptr);
	}

	[[nodiscard]] auto log() const -> const std::string & { return log_; }

private:
	static constexpr int kMaxThreads = 4;

	auto next(int tid) const -> Event { return {tid, g_->getThreadSize(tid)}; }

	auto add(std::unique_ptr<EventLabel> lab) -> EventLabel *
	{
		log_ += std::format("add {} {}\n", lab->getKind(), lab->getPos());
		lab->setViews({View()});
		return g_->add(std::move(lab));
	}

	/** A thread that may get events: one whose begin is not terminated */
	auto liveThread(Draws &d) const -> std::optional<int>
	{
		std::vector<int> live;
		for (auto t = 0; t < g_->getNumThreads(); t++) {
			const auto *bLab = genmc::dyn_cast<ThreadStartLabel>(g_->getFirstThreadLabel(t));
			if (t == 0 || (bLab && !bLab->isTerminated()))
				live.push_back(t);
		}
		return live[d.below(live.size())];
	}

	void read(int t, Draws &d)
	{
		auto addr = addrs_[d.below(addrs_.size())];
		std::vector<EventLabel *> writes{g_->getInitLabel()};
		for (auto &lab : g_->labels())
			if (auto *wLab = genmc::dyn_cast<WriteLabel>(&lab); wLab && wLab->getAddr() == addr)
				writes.push_back(wLab);
		auto *rf = writes[d.below(writes.size())];
		auto *rLab = genmc::cast<ReadLabel>(add(ReadLabel::create(
			next(t), MemOrdering::Relaxed, addr, ASize(4), nullptr, std::nullopt)));
		rLab->setRf(rf);
	}

	void create(int t)
	{
		auto id = g_->getNumThreads();
		auto info = ThreadInfo{id, t, 0, SVal(0), "t"};
		auto *tc = genmc::cast<ThreadCreateLabel>(add(ThreadCreateLabel::create(next(t), info)));
		g_->addNewThread();
		add(ThreadStartLabel::create(Event(id, 0), tc->getPos(), tc, info));
	}

	/** The last label of a thread goes, if a read or a fence (removing
	 * a write leaves it in coherence; no production caller does) that
	 * does not follow a hole (blocking and unblocking remove labels just
	 * added, after the kept ones; a cut drops trailing holes itself) */
	void removeLast(Draws &d)
	{
		auto t = d.below(g_->getNumThreads());
		const auto *lab = g_->getLastThreadLabel(t);
		if (lab->getIndex() == 0 ||
		    !(genmc::isa<FenceLabel>(lab) || genmc::isa<ReadLabel>(lab)) ||
		    genmc::isa<EmptyLabel>(g_->po_imm_pred(lab)))
			return;
		log_ += std::format("removeLast {}\n", lab->getPos());
		g_->removeLast(t);
	}

	void cut(Draws &d)
	{
		auto st = Stamp(d.below(g_->getMaxStamp().get() + 1));
		log_ += std::format("cutToStamp {}\n", st.get());
		auto plan = invariants::planCutToStamp(*g_, st);
		g_->cutToStamp(st);
		check(&plan);
	}

	/** Copies up to a random view closed under rf and thread creation (and
	 * po, without dependency tracking), as the views of revisits are */
	void copy(Draws &d)
	{
		std::set<Event> keep;
		std::vector<Event> work;
		for (const auto &lab : g_->labels())
			if (lab.getIndex() > 0 && !genmc::isa<EmptyLabel>(&lab) && d.below(2) == 0)
				work.push_back(lab.getPos());
		while (!work.empty()) {
			auto e = work.back();
			work.pop_back();
			if (e.isInitializer() || !keep.insert(e).second)
				continue;
			const auto *lab = g_->getEventLabel(e);
			if (const auto *rLab = genmc::dyn_cast<ReadLabel>(lab))
				work.push_back(rLab->getRf()->getPos());
			if (e.index > 0) {
				const auto *bLab = genmc::cast<ThreadStartLabel>(
					g_->getFirstThreadLabel(e.thread));
				work.push_back(bLab->getCreateId());
			}
			if (!dep_ && e.index > 1)
				work.push_back(e.prev());
		}
		auto v = view(keep);
		log_ += std::format("copy up to {}\n", *v);
		invariants::checkCopyView(*g_, *v, ctx());
		auto plan = invariants::planCopyUpTo(*g_, *v);
		g_ = g_->getCopyUpTo(*v);
		check(&plan);
	}

	/** Opens slots in a thread, or closes slots that hold holes */
	void relocate(Draws &d)
	{
		auto t = static_cast<int>(d.below(g_->getNumThreads()));
		auto size = g_->getThreadSize(t);
		if (size < 2)
			return;
		auto from = 1 + static_cast<int>(d.below(size - 1));
		if (d.below(2) == 0) {
			auto delta = 1 + static_cast<int>(d.below(2));
			log_ += std::format("relocate thread {} from {} by {}\n", t, from, delta);
			g_->relocateThread(t, from, delta);
			return;
		}
		auto delta = 0;
		while (from + delta - 1 > 0 &&
		       genmc::isa<EmptyLabel>(g_->getEventLabel(Event(t, from + delta - 1))))
			--delta;
		if (delta == 0)
			return;
		log_ += std::format("relocate thread {} from {} by {}\n", t, from, delta);
		g_->relocateThread(t, from, delta);
	}

	/** Caches the porf-prefix of a label as its prefix view */
	void cachePrefix(Draws &d)
	{
		std::vector<const EventLabel *> labs;
		for (const auto &lab : g_->labels())
			if (lab.getIndex() > 0 && !genmc::isa<EmptyLabel>(&lab))
				labs.push_back(&lab);
		if (labs.empty())
			return;
		const auto *lab = labs[d.below(labs.size())];
		std::set<Event> prefix;
		std::vector<Event> work{lab->getPos()};
		while (!work.empty()) {
			auto e = work.back();
			work.pop_back();
			if (e.isInitializer() || !prefix.insert(e).second)
				continue;
			const auto *eLab = g_->getEventLabel(e);
			if (const auto *rLab = genmc::dyn_cast<ReadLabel>(eLab))
				work.push_back(rLab->getRf()->getPos());
			if (const auto *bLab = genmc::dyn_cast<ThreadStartLabel>(eLab))
				work.push_back(bLab->getCreateId());
			for (auto i = e.index - 1; i >= 0; i--)
				if (!genmc::isa<EmptyLabel>(g_->getEventLabel(Event(e.thread, i))))
					work.push_back(Event(e.thread, i));
		}
		auto v = view(prefix);
		log_ += std::format("cache the prefix view of {}: {}\n", lab->getPos(), *v);
		lab->setPrefixView(std::move(v));
	}

	/** The view of the events of KEEP (with holes for the others, under
	 * dependency tracking); every thread's begin is in it */
	auto view(const std::set<Event> &keep) const -> std::unique_ptr<VectorClock>
	{
		std::unique_ptr<VectorClock> v;
		if (dep_)
			v = std::make_unique<DepView>();
		else
			v = std::make_unique<View>();
		for (auto t = 0; t < g_->getNumThreads(); t++)
			v->updateIdx(Event(t, 0));
		for (const auto &e : keep)
			v->updateIdx(e);
		return v;
	}

	auto ctx() const -> invariants::Context
	{
		return {.where = "a generated operation", .holesAllowed = dep_};
	}

	/** I1, and the cached prefix views name events of the graph */
	void check(const invariants::RestrictionPlan *plan)
	{
		invariants::checkGraph(*g_, ctx(), plan);
		for (const auto &lab : g_->labels()) {
			if (!lab.hasPrefixView() || genmc::isa<InitLabel>(&lab))
				continue;
			const auto &pv = lab.getPrefixView();
			for (auto t = 0; t < pv.size(); t++)
				for (auto i = 1; i <= pv.getMax(t); i++) {
					auto e = Event(t, i);
					if (!pv.contains(e))
						continue;
					const auto *eLab = g_->getEventLabelIfPresent(e);
					if (!eLab || genmc::isa<EmptyLabel>(eLab))
						throw invariants::Violation(std::format(
							"the prefix view of {} names {}, which "
							"the graph {}",
							lab.getPos(), e,
							eLab ? "has as a hole" : "lacks"));
				}
		}
	}

	bool dep_;
	StubChecker checker_;
	std::unique_ptr<ExecutionGraph> g_;
	std::vector<SAddr> addrs_{SAddr::createHeap(0, 1, false, false),
				  SAddr::createHeap(0, 2, false, false)};
	std::string log_;
};

void runOperations(bool depTracking, const std::vector<unsigned> &vals)
{
	invariants::ThrowOnViolation throwing;
	Harness h(depTracking);
	Draws d(vals);
	try {
		while (!d.done())
			h.step(d);
	} catch (const invariants::Violation &v) {
		RC_FAIL(std::format("{}\nafter:\n{}", v.what(), h.log()));
	}
}

} // namespace

RC_GTEST_PROP(GraphProperty, OperationsKeepTheInvariants, ())
{
	runOperations(false, *rc::gen::container<std::vector<unsigned>>(rc::gen::arbitrary<unsigned>()));
}

RC_GTEST_PROP(GraphProperty, OperationsKeepTheInvariantsWithHoles, ())
{
	runOperations(true, *rc::gen::container<std::vector<unsigned>>(rc::gen::arbitrary<unsigned>()));
}
