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

#include "genmc/Verification/Checks/Invariants.hpp"
#include "genmc/ADT/VectorClock.hpp"
#include "genmc/ADT/View.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Execution/ExecutionState.hpp"
#include "genmc/Support/ASize.hpp"
#include "genmc/Support/Cast.hpp"
#include "genmc/Support/Error.hpp"
#include "genmc/Support/Hash.hpp"
#include "genmc/Support/SAddr.hpp"
#include "genmc/Verification/Report/GraphPrinting.hpp"

#include <algorithm>
#include <format>
#include <iostream>
#include <optional>
#include <ranges>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace invariants {

void fail(std::string_view invariant, const std::string &what, const ExecutionGraph *g,
	  const Context &ctx, std::source_location loc)
{
	auto header = std::format("Invariant {} violated after {}: {}", invariant, ctx.where, what);
	/* Say what failed before touching the graph: printing a corrupted graph
	 * may itself crash */
	std::cerr << "\n" << header << "\n" << std::flush;

	std::ostringstream out;
	out << header << "\n";
	if (g) {
		out << "in graph:\n";
		if (ctx.dbgInfo)
			printGraph(*g, *ctx.dbgInfo, out);
		else
			out << std::format("{}", *g);
	}
	genmc::detail::report_internal_error(out.str(), loc);
}

namespace {

constexpr std::string_view kI1 = "I1 (restriction)";
constexpr std::string_view kI3 = "I3 (allocation map)";
constexpr std::string_view kI4 = "I4 (resume point)";
constexpr std::string_view kI5 = "I5 (consistency)";
constexpr std::string_view kI6 = "I6 (error graph)";

/** Whether LAB, a pointer another label stores, is a label of G */
auto inGraph(const ExecutionGraph &g, const EventLabel *lab) -> bool
{
	return lab != nullptr && g.containsLab(lab);
}

/** The thread an address was allocated by, and the offset in its pool */
auto addrThread(SAddr addr) -> int
{
	return static_cast<int>((addr.get() & SAddr::threadMask) >> SAddr::threadStartBit);
}
auto addrIndex(SAddr addr) -> SAddr::Width { return addr.get() & SAddr::indexMask; }

/** Address space a block takes (a zero-size block takes a byte) */
auto blockSpan(const MallocLabel *aLab) -> uint64_t { return std::max(aLab->getSize().get(), 1UL); }

/*******************************************************************************
 **                        I1: the restricted graph
 ******************************************************************************/

/** Thread ids are dense; begins and their creates agree */
void checkThreads(const ExecutionGraph &g, const Context &ctx)
{
	const auto n = g.getNumThreads();
	const auto pruned = g.wasPruned();
	auto failI1 = [&](std::string what) { fail(kI1, what, &g, ctx); };

	if (n < 1)
		failI1("the graph has no threads");
	for (auto t : g.thr_ids()) {
		if (g.isThreadEmpty(t))
			failI1(std::format("thread {} has no labels", t));
		/* A hole stands for an event before ones a revisit kept (#180) */
		if (genmc::isa<EmptyLabel>(g.getLastThreadLabel(t)))
			failI1(std::format("thread {} ends with a hole", t));
		const auto *first = g.getEventLabelIfPresent(Event(t, 0));
		if (first == nullptr)
			failI1(std::format("thread {} has no label at index 0", t));
		if (t == 0) {
			if (!genmc::isa<InitLabel>(first))
				failI1("thread 0 does not start with the initializer");
			continue;
		}
		const auto *bLab = genmc::dyn_cast<ThreadStartLabel>(first);
		if (bLab == nullptr)
			failI1(std::format("thread {} does not start with a begin label", t));
		if (bLab->getThreadInfo().id != t)
			failI1(std::format("the begin of thread {} carries thread id {}", t,
					   bLab->getThreadInfo().id));
		const auto *tcLab = bLab->getCreate();
		if (tcLab != nullptr) {
			if (!inGraph(g, tcLab))
				failI1(std::format(
					"the begin of thread {} points at a create ({}) that is "
					"not in the graph",
					t, bLab->getCreateId()));
			if (tcLab->getPos() != bLab->getCreateId())
				failI1(std::format("the begin of thread {} records its create at "
						   "{} but points at {}",
						   t, bLab->getCreateId(), tcLab->getPos()));
			if (tcLab->getChildId() != t)
				failI1(std::format("the create {} of thread {} names thread {} as "
						   "its child",
						   tcLab->getPos(), t, tcLab->getChildId()));
			if (!pruned && bLab->isTerminated())
				failI1(std::format("the begin of thread {} is terminated although "
						   "its create {} is in the graph",
						   t, tcLab->getPos()));
			continue;
		}
		/* Pruning (random mode) keeps a thread's labels without its create */
		if (pruned)
			continue;
		if (g.getThreadSize(t) != 1)
			failI1(std::format("thread {} has {} labels although its create {} was cut",
					   t, g.getThreadSize(t), bLab->getCreateId()));
		if (!bLab->isTerminated())
			failI1(std::format("the begin of thread {} lost its create {} but is not "
					   "terminated",
					   t, bLab->getCreateId()));
		if (t == n - 1)
			failI1(std::format("thread {} only has a begin whose create {} was cut, "
					   "and is the last thread (it should have been erased)",
					   t, bLab->getCreateId()));
	}
}

/** Every label is found at its position; po lists, insertion order and
 * stamps agree; holes only where allowed */
void checkLabels(const ExecutionGraph &g, const Context &ctx, bool restricted)
{
	const auto pruned = g.wasPruned();
	auto failI1 = [&](std::string what) { fail(kI1, what, &g, ctx); };

	std::size_t count = 0;
	std::optional<Stamp> prev;
	for (const auto &lab : g.labels()) {
		++count;
		if (lab.getParent() != &g)
			failI1(std::format("label {} belongs to another graph", lab.getPos()));
		if (!lab.hasStamp())
			failI1(std::format("label {} has no stamp", lab.getPos()));
		if (prev && !(*prev < lab.getStamp()))
			failI1(std::format("stamps do not increase along the insertion order at "
					   "{} ({} after {})",
					   lab.getPos(), lab.getStamp(), *prev));
		prev = lab.getStamp();
		if (g.getEventLabelIfPresent(lab.getPos()) != &lab)
			failI1(std::format("label {} is not the label found at its position",
					   lab.getPos()));
	}
	if (prev && *prev != g.getMaxStamp())
		failI1(std::format("the maximum stamp is {} but the newest label has stamp {}",
				   g.getMaxStamp(), *prev));
	if (restricted && prev && prev->get() + 1 != count)
		failI1(std::format("stamps are not dense after the restriction: {} labels, "
				   "maximum stamp {}",
				   count, *prev));

	std::size_t poCount = 0;
	for (auto t : g.thr_ids()) {
		auto lastIdx = -1;
		auto rank = 0;
		for (const auto &lab : g.po(t)) {
			++poCount;
			if (lab.getThread() != t)
				failI1(std::format("label {} is in the po list of thread {}",
						   lab.getPos(), t));
			if (lab.getIndex() <= lastIdx)
				failI1(std::format(
					"the po list of thread {} does not increase at {}", t,
					lab.getPos()));
			if (!pruned && lab.getIndex() != rank)
				failI1(std::format("label {} sits at slot {} of thread {}",
						   lab.getPos(), rank, t));
			if (genmc::isa<EmptyLabel>(&lab)) {
				if (!ctx.holesAllowed)
					failI1(std::format(
						"hole at {} in a graph without dependency "
						"tracking",
						lab.getPos()));
				if (lab.getIndex() == 0)
					failI1(std::format("hole at {} in place of a begin",
							   lab.getPos()));
			}
			lastIdx = lab.getIndex();
			++rank;
		}
		if (rank != g.getThreadSize(t))
			failI1(std::format("the po list of thread {} has {} labels but the thread "
					   "has {}",
					   t, rank, g.getThreadSize(t)));
	}
	if (poCount != count)
		failI1(std::format("the insertion order has {} labels, the threads have {}", count,
				   poCount));
}

/** rf, readers, coherence and the initializer's readers point at labels of G */
void checkRelations(const ExecutionGraph &g, const Context &ctx)
{
	auto failI1 = [&](std::string what) { fail(kI1, what, &g, ctx); };

	for (const auto &lab : g.labels()) {
		if (const auto *rLab = genmc::dyn_cast<ReadLabel>(&lab)) {
			const auto *rf = rLab->getRf();
			if (rf == nullptr)
				continue; /* repaired after the restriction */
			if (!inGraph(g, rf))
				failI1(std::format("read {} reads from a label that is not in the "
						   "graph",
						   rLab->getPos()));
			if (const auto *wLab = genmc::dyn_cast<WriteLabel>(rf)) {
				if (wLab->getAddr() != rLab->getAddr())
					failI1(std::format("read {} reads from {}, a write to "
							   "another address",
							   rLab->getPos(), wLab->getPos()));
				if (std::ranges::none_of(wLab->readers(), [rLab](auto &oLab) {
					    return &oLab == rLab;
				    }))
					failI1(std::format("read {} is missing from the readers of "
							   "its rf source {}",
							   rLab->getPos(), wLab->getPos()));
			} else if (genmc::isa<InitLabel>(rf)) {
				const auto *iLab = g.getInitLabel();
				if (!iLab->hasRfs(rLab->getAddr()) ||
				    std::ranges::none_of(
					    iLab->rfs(rLab->getAddr()),
					    [rLab](auto &oLab) { return &oLab == rLab; }))
					failI1(std::format("read {} reads from the initializer but "
							   "is missing from its readers",
							   rLab->getPos()));
			} else {
				failI1(std::format("read {} reads from {}, which is not a write",
						   rLab->getPos(), rf->getPos()));
			}
		}
		if (const auto *wLab = genmc::dyn_cast<WriteLabel>(&lab)) {
			for (const auto &oLab : wLab->readers()) {
				if (!inGraph(g, &oLab))
					failI1(std::format("write {} has a reader that is not in "
							   "the graph",
							   wLab->getPos()));
				if (oLab.getRf() != wLab)
					failI1(std::format("reader {} of write {} reads from "
							   "elsewhere",
							   oLab.getPos(), wLab->getPos()));
			}
		}
	}

	std::unordered_set<Event> inCo;
	for (auto lIt = g.loc_begin(); lIt != g.loc_end(); ++lIt) {
		for (const auto &wLab : g.co(lIt->first)) {
			if (!inGraph(g, &wLab))
				failI1(std::format("the coherence order of {} lists a write that "
						   "is not in the graph",
						   lIt->first));
			if (wLab.getAddr() != lIt->first)
				failI1(std::format("write {} to {} is in the coherence order of {}",
						   wLab.getPos(), wLab.getAddr(), lIt->first));
			if (!inCo.insert(wLab.getPos()).second)
				failI1(std::format("write {} appears twice in the coherence orders",
						   wLab.getPos()));
		}
		const auto *iLab = g.getInitLabel();
		if (!iLab->hasRfs(lIt->first))
			continue;
		for (const auto &rLab : iLab->rfs(lIt->first)) {
			if (!inGraph(g, &rLab))
				failI1(std::format("the initializer's readers of {} include a "
						   "label that is not in the graph",
						   lIt->first));
			if (!genmc::isa<InitLabel>(rLab.getRf()))
				failI1(std::format("read {} is among the initializer's readers "
						   "but reads from elsewhere",
						   rLab.getPos()));
		}
	}
}

/** create <-> begin and join <-> finish agree */
void checkThreadLinks(const ExecutionGraph &g, const Context &ctx)
{
	const auto n = g.getNumThreads();
	auto failI1 = [&](std::string what) { fail(kI1, what, &g, ctx); };

	for (const auto &lab : g.labels()) {
		if (const auto *tcLab = genmc::dyn_cast<ThreadCreateLabel>(&lab)) {
			auto cid = tcLab->getChildId();
			if (cid <= 0 || cid >= n)
				failI1(std::format("create {} names thread {} as its child, which "
						   "the graph does not have",
						   tcLab->getPos(), cid));
			if (!g.wasPruned() && g.getFirstThreadLabel(cid)->getCreate() != tcLab)
				failI1(std::format("the begin of thread {} is not linked to its "
						   "create {}",
						   cid, tcLab->getPos()));
		}
		if (const auto *eLab = genmc::dyn_cast<ThreadFinishLabel>(&lab)) {
			const auto *jLab = eLab->getParentJoin();
			if (jLab == nullptr)
				continue;
			if (!inGraph(g, jLab))
				failI1(std::format("finish {} points at a join that is not in the "
						   "graph",
						   eLab->getPos()));
			if (jLab->getChildId() != eLab->getThread())
				failI1(std::format("finish {} points at join {}, which joins "
						   "thread {}",
						   eLab->getPos(), jLab->getPos(),
						   jLab->getChildId()));
		}
		if (const auto *jLab = genmc::dyn_cast<ThreadJoinLabel>(&lab)) {
			auto cid = jLab->getChildId();
			/* An invalid tid is a user error that the driver reports */
			if (cid < 0 || cid >= n || cid == jLab->getThread())
				continue;
			const auto *eLab = genmc::dyn_cast_if_present<ThreadFinishLabel>(
				g.getLastThreadLabel(cid));
			if (eLab == nullptr)
				failI1(std::format("join {} of thread {}, whose last label is not "
						   "a finish",
						   jLab->getPos(), cid));
			if (eLab->getParentJoin() != jLab)
				failI1(std::format("the finish {} of thread {} is not linked to "
						   "its join {}",
						   eLab->getPos(), cid, jLab->getPos()));
		}
	}
}

/** Cached prefix views and calculated views stay within the threads */
void checkViews(const ExecutionGraph &g, const Context &ctx)
{
	const auto n = g.getNumThreads();
	auto failI1 = [&](std::string what) { fail(kI1, what, &g, ctx); };

	/* Pruning keeps positions, so a view may legitimately exceed a thread's size */
	if (g.wasPruned())
		return;

	auto checkBounds = [&](const EventLabel &lab, const auto &view, std::string_view kind) {
		for (auto t = 0; t < view.size(); t++) {
			if (t >= n || view.getMax(t) >= g.getThreadSize(t))
				failI1(std::format("the {} of {} reaches ({}, {}), beyond thread "
						   "{}'s {} labels",
						   kind, lab.getPos(), t, view.getMax(t), t,
						   t < n ? g.getThreadSize(t) : 0));
		}
	};
	for (const auto &lab : g.labels()) {
		/* The views of a begin whose create was cut are stale (see PR #32) */
		if (const auto *bLab = genmc::dyn_cast<ThreadStartLabel>(&lab);
		    bLab && !genmc::isa<InitLabel>(bLab) && bLab->getCreate() == nullptr)
			continue;
		/* A read whose rf source the restriction cut (the revisited one, or
		 * a dangling one the driver repairs) has its views recomputed with
		 * its new rf; the cached ones are stale until then */
		if (const auto *rLab = genmc::dyn_cast<ReadLabel>(&lab);
		    rLab && rLab->getRf() == nullptr)
			continue;
		if (lab.hasPrefixView())
			checkBounds(lab, lab.getPrefixView(), "prefix view");
		/* Under a dependency-tracking model the prefix a backward revisit
		 * copies is not po-closed: a kept label's hb view may name events
		 * of a thread whose events were not copied, which the thread adds
		 * again. Only a plain graph's views stay within it. */
		if (ctx.holesAllowed)
			continue;
		for (const auto &view : lab.views())
			checkBounds(lab, view, "view");
	}
}

/** The restriction kept exactly the planned labels */
void checkPlan(const ExecutionGraph &g, const Context &ctx, const RestrictionPlan &plan)
{
	const auto n = g.getNumThreads();
	auto failI1 = [&](std::string what) { fail(kI1, what, &g, ctx); };

	std::unordered_set<Event> expected(plan.kept.begin(), plan.kept.end());
	for (const auto &lab : g.labels()) {
		if (genmc::isa<EmptyLabel>(&lab))
			continue;
		if (!expected.contains(lab.getPos()))
			failI1(std::format(
				"label {} survived the {} although it was not to be kept",
				lab.getPos(), plan.cut));
	}
	for (const auto &e : plan.kept) {
		if (g.containsPosNonEmpty(e))
			continue;
		/* Only the begin of an erased trailing thread may go, and only
		 * if its create went too */
		if (e.index != 0 || e.thread < n)
			failI1(std::format("label {} was to be kept by the {} but is gone", e,
					   plan.cut));
		if (std::cmp_less(e.thread, plan.createKept.size()) && plan.createKept[e.thread])
			failI1(std::format("thread {} was erased by the {} although its create "
					   "was kept",
					   e.thread, plan.cut));
	}
}

/*******************************************************************************
 **                        I4: fingerprints
 ******************************************************************************/

auto hashLabel(const ExecutionGraph &g, const EventLabel &lab, Event saved) -> std::size_t
{
	std::size_t h = 0;
	hash_combine(h, static_cast<int>(lab.getKind()));
	hash_combine(h, lab.getPos());
	if (const auto *mLab = genmc::dyn_cast<MemAccessLabel>(&lab)) {
		hash_combine(h, mLab->getAddr().get());
		hash_combine(h, mLab->getSize().get());
	}
	if (const auto *aLab = genmc::dyn_cast<MallocLabel>(&lab)) {
		hash_combine(h, aLab->getAddr().get());
		hash_combine(h, aLab->getSize().get());
	}
	if (const auto *dLab = genmc::dyn_cast<FreeLabel>(&lab))
		hash_combine(h, dLab->getAddr().get());
	if (lab.getPos() != saved) {
		if (const auto *rLab = genmc::dyn_cast<ReadLabel>(&lab))
			hash_combine(h, rLab->getRf() ? rLab->getRf()->getPos() : Event(-1, -1));
		if (const auto *wLab = genmc::dyn_cast<WriteLabel>(&lab))
			hash_combine(h, wLab->getVal().get());
	}
	if (const auto *bLab = genmc::dyn_cast<ThreadStartLabel>(&lab)) {
		hash_combine(h, bLab->getCreateId());
		hash_combine(h, bLab->getCreate() != nullptr);
		hash_combine(h, bLab->isTerminated());
	}
	if (const auto *tcLab = genmc::dyn_cast<ThreadCreateLabel>(&lab))
		hash_combine(h, tcLab->getChildId());
	if (const auto *jLab = genmc::dyn_cast<ThreadJoinLabel>(&lab))
		hash_combine(h, jLab->getChildId());
	return h;
}

} // namespace

auto planCutToStamp(const ExecutionGraph &g, Stamp st) -> RestrictionPlan
{
	RestrictionPlan plan;
	plan.cut = std::format("cut to stamp {}", st);
	for (const auto &lab : g.labels()) {
		if (genmc::isa<EmptyLabel>(&lab))
			continue;
		if (lab.getStamp() <= st || lab.getIndex() == 0)
			plan.kept.push_back(lab.getPos());
	}
	for (auto t : g.thr_ids()) {
		const auto *bLab = genmc::dyn_cast_if_present<ThreadStartLabel>(
			g.getEventLabelIfPresent(Event(t, 0)));
		const auto *tcLab = bLab ? bLab->getCreate() : nullptr;
		plan.createKept.push_back(tcLab && tcLab->getStamp() <= st);
	}
	return plan;
}

auto planCopyUpTo(const ExecutionGraph &g, const VectorClock &v) -> RestrictionPlan
{
	RestrictionPlan plan;
	plan.cut = "copy up to a view";
	for (const auto &lab : g.labels()) {
		if (genmc::isa<EmptyLabel>(&lab))
			continue;
		if (v.contains(lab.getPos()) || lab.getIndex() == 0)
			plan.kept.push_back(lab.getPos());
	}
	for (auto t : g.thr_ids()) {
		const auto *bLab = genmc::dyn_cast_if_present<ThreadStartLabel>(
			g.getEventLabelIfPresent(Event(t, 0)));
		const auto *tcLab = bLab ? bLab->getCreate() : nullptr;
		plan.createKept.push_back(tcLab && v.contains(tcLab->getPos()) &&
					  g.containsPosNonEmpty(tcLab->getPos()));
	}
	return plan;
}

void checkCopyView(const ExecutionGraph &g, const VectorClock &v, const Context &ctx)
{
	const auto n = g.getNumThreads();
	for (auto t = 0; t < v.size(); t++) {
		if (t >= n || v.getMax(t) >= g.getThreadSize(t))
			fail(kI1,
			     std::format("the view to copy reaches ({}, {}), beyond thread {}'s "
					 "{} labels",
					 t, v.getMax(t), t, t < n ? g.getThreadSize(t) : 0),
			     &g, ctx);
	}
}

void checkGraph(const ExecutionGraph &g, const Context &ctx, const RestrictionPlan *plan)
{
	checkThreads(g, ctx);
	checkLabels(g, ctx, plan != nullptr);
	checkRelations(g, ctx);
	checkThreadLinks(g, ctx);
	checkViews(g, ctx);
	if (plan != nullptr)
		checkPlan(g, ctx, *plan);
}

/*******************************************************************************
 **                        I3: the allocation map
 ******************************************************************************/

void checkAllocations(const ExecutionGraph &g, const ExecutionState &state, const Context &ctx)
{
	auto failI3 = [&](std::string what) { fail(kI3, what, &g, ctx); };

	std::unordered_map<SAddr, const MallocLabel *> blocks;
	for (const auto &lab : g.labels()) {
		if (const auto *aLab = genmc::dyn_cast<MallocLabel>(&lab)) {
			auto addr = aLab->getAddr();
			if (addr == SAddr()) /* a failed allocation */
				continue;
			if (auto [it, fresh] = blocks.emplace(addr, aLab); !fresh)
				failI3(std::format("allocations {} and {} have the same address {}",
						   it->second->getPos(), aLab->getPos(), addr));
			if (!state.isAllocated(addr))
				failI3(std::format("the block {} of {} is not registered", addr,
						   aLab->getPos()));
			if (state.getAllocPos(addr) != aLab->getPos())
				failI3(std::format(
					"the block {} is registered for {} instead of {}", addr,
					state.getAllocPos(addr), aLab->getPos()));
			auto access = state.getAllocAccess(addr);
			if (access.addr != addr || access.size != aLab->getSize())
				failI3(std::format("the block of {} is registered as [{}, +{}) "
						   "instead of [{}, +{})",
						   aLab->getPos(), access.addr, access.size.get(),
						   addr, aLab->getSize().get()));
			if (!addr.isDynamic())
				failI3(std::format("allocation {} has a static address {}",
						   aLab->getPos(), addr));
			if (addrThread(addr) != aLab->getThread())
				failI3(std::format("allocation {} has an address of thread {}: {}",
						   aLab->getPos(), addrThread(addr), addr));
			auto end = addrIndex(addr) + blockSpan(aLab);
			if (state.allocatorPoolEnd(aLab->getThread()) < end)
				failI3(std::format("thread {}'s allocator pool ({:#x}) lies before "
						   "the end of its block {} of {} ({:#x})",
						   aLab->getThread(),
						   state.allocatorPoolEnd(aLab->getThread()), addr,
						   aLab->getPos(), end));
		}
		if (const auto *dLab = genmc::dyn_cast<FreeLabel>(&lab)) {
			/* An invalid free (an error event) frees no block */
			auto addr = dLab->getAddr();
			auto isBlockStart = state.isAllocated(addr) &&
					    state.getAllocAccess(addr).addr == addr;
			if (isBlockStart && !state.isFreed(addr))
				failI3(std::format(
					"free {} of {}, which is not registered as freed",
					dLab->getPos(), addr));
		}
	}
	for (const auto &entry : state.allocations()) {
		const auto &info = entry.second;
		const auto *aLab =
			genmc::dyn_cast_if_present<MallocLabel>(g.getEventLabelIfPresent(info.pos));
		if (aLab == nullptr)
			failI3(std::format("a block at {} is registered for {}, which is not an "
					   "allocation of the graph",
					   entry.first.start, info.pos));
		auto start = aLab->getAddr();
		auto end = start + ASize(blockSpan(aLab));
		if (entry.first.start != start || entry.first.end != end)
			failI3(std::format("the block of {} spans [{}, {}) in the allocation map "
					   "but [{}, {}) in the graph",
					   info.pos, entry.first.start, entry.first.end, start,
					   end));
		if (!info.freePos)
			continue;
		const auto *dLab = genmc::dyn_cast_if_present<FreeLabel>(
			g.getEventLabelIfPresent(*info.freePos));
		if (dLab == nullptr || dLab->getAddr() != start)
			failI3(std::format("the block of {} is marked freed by {}, which is not a "
					   "free of it",
					   info.pos, *info.freePos));
	}
}

/*******************************************************************************
 **                        I4: resume points
 ******************************************************************************/

auto fingerprint(const ExecutionGraph &g, Event saved) -> GraphFingerprint
{
	GraphFingerprint fp;
	fp.saved = saved;
	fp.maxStamp = g.getMaxStamp();
	for (auto t : g.thr_ids()) {
		/* Trailing holes and block labels are left out. A cut to the
		 * stamp of the save drops a trailing hole filled since (holes have
		 * stamps, and the new label a newer one), and a block label
		 * removed since (the thread was unblocked and retried its
		 * instruction, which added a newer label): the thread of the saved
		 * state then retries the instruction in the resumed execution, as
		 * it would in a replayed one */
		auto last = -1;
		for (const auto &lab : g.po(t))
			if (!genmc::isa<EmptyLabel>(&lab) && !genmc::isa<BlockLabel>(&lab))
				last = lab.getIndex();
		std::size_t h = 0;
		for (const auto &lab : g.po(t))
			if (lab.getIndex() <= last)
				hash_combine(h, hashLabel(g, lab, saved));
		fp.lastIndex.push_back(last);
		fp.threadHash.push_back(h);
	}

	/* Coherence, in a fixed location order. (A location without writes
	 * is left out: a cut keeps the entry of a location that only labels
	 * it turns into holes accessed, which the graph at the save lacked) */
	for (auto lIt = g.loc_begin(); lIt != g.loc_end(); ++lIt) {
		std::size_t h = 0;
		auto writes = 0U;
		for (const auto &wLab : g.co(lIt->first)) {
			if (wLab.getPos() == saved)
				continue;
			hash_combine(h, wLab.getPos());
			++writes;
		}
		if (writes > 0)
			fp.coHashes.emplace_back(lIt->first.get(), h);
	}
	std::ranges::sort(fp.coHashes);
	return fp;
}

auto resumeMismatch(const ExecutionGraph &g, const GraphFingerprint &saved,
		    const std::vector<Event> &positions) -> std::optional<std::string>
{
	std::optional<std::string> mismatch;
	auto failI4 = [&](std::string what) {
		if (!mismatch)
			mismatch = std::move(what);
	};
	auto now = fingerprint(g, saved.saved);
	auto prefix = std::format("resuming the revisit of {} (state saved at stamp {})",
				  saved.saved, saved.maxStamp);

	if (now.lastIndex.size() != saved.lastIndex.size()) {
		failI4(std::format("{}: the restricted graph has {} threads, the graph at the "
				   "save had {}",
				   prefix, now.lastIndex.size(), saved.lastIndex.size()));
		return mismatch;
	}
	for (auto t = 0U; t < now.lastIndex.size(); t++) {
		if (now.lastIndex[t] != saved.lastIndex[t])
			failI4(std::format(
				"{}: thread {} ends at index {} in the restricted graph, "
				"at {} in the graph at the save",
				prefix, t, now.lastIndex[t], saved.lastIndex[t]));
	}
	for (auto t = 0U; t < now.threadHash.size(); t++) {
		if (now.threadHash[t] != saved.threadHash[t])
			failI4(std::format("{}: the labels of thread {} differ from those of the "
					   "graph at the save",
					   prefix, t));
	}
	for (auto i = 0U; i < std::max(now.coHashes.size(), saved.coHashes.size()); i++) {
		if (i >= now.coHashes.size() || i >= saved.coHashes.size() ||
		    now.coHashes[i] != saved.coHashes[i]) {
			auto addr = i < now.coHashes.size() ? now.coHashes[i].first
							    : saved.coHashes[i].first;
			failI4(std::format("{}: the coherence order of {} differs from that of "
					   "the graph at the save",
					   prefix, SAddr(addr)));
			break;
		}
	}
	if (mismatch)
		return mismatch;

	/* The state's threads against the graph (the runtime declines a
	 * state with a thread past the graph, see Interpreter::resumeFrom()) */
	if (positions.size() > now.lastIndex.size())
		return mismatch;
	for (auto t = 0U; t < positions.size(); t++)
		if (positions[t].index > now.lastIndex[t])
			return mismatch;
	for (auto t = 0U; t < positions.size(); t++) {
		if (positions[t].thread != static_cast<int>(t))
			failI4(std::format("{}: the state's thread {} is at {}", prefix, t,
					   positions[t]));
		for (auto j = 1; j <= positions[t].index; j++) {
			if (genmc::isa<EmptyLabel>(g.getEventLabel(Event(t, j))))
				failI4(std::format(
					"{}: the state's thread {} is at {} but the "
					"restricted graph has a hole at ({}, {}) before it",
					prefix, t, positions[t], t, j));
		}
	}
	return mismatch;
}

void checkConsistency(const ExecutionGraph &g, bool consistent, const Context &ctx)
{
	if (!consistent)
		fail(kI5, "the execution violates the memory model's axioms (whole-graph check)", &g,
		     ctx);
}

void checkErrorConsistency(const ExecutionGraph &g, bool consistent, const Context &ctx)
{
	if (!consistent)
		fail(kI6,
		     "the graph of the error is inconsistent once its open read-side critical "
		     "sections are closed: no execution has it",
		     &g, ctx);
}

void checkResume(const ExecutionGraph &g, const GraphFingerprint &saved,
		 const std::vector<Event> &positions, const Context &ctx)
{
	if (auto mismatch = resumeMismatch(g, saved, positions))
		fail(kI4, *mismatch, &g, ctx);
}

void checkResumeSchedule(const ExecutionGraph &g, const std::vector<Event> &schedule,
			 const Context &ctx)
{
	for (auto t : g.thr_ids()) {
		auto hole = -1;
		for (const auto &lab : g.po(t)) {
			if (genmc::isa<EmptyLabel>(&lab)) {
				if (hole < 0)
					hole = lab.getIndex();
				continue;
			}
			/* A replay is driven through the atomic kept events only
			 * (the schedule leaves out non-atomic ones and blocks) */
			if (hole < 0 || lab.isNotAtomic() || genmc::isa<BlockLabel>(&lab))
				continue;
			if (std::ranges::none_of(schedule, [&](const Event &e) {
				    return e.thread == t && e.index > hole;
			    }))
				fail(kI4,
				     std::format("the resumed execution is not driven through the "
						 "hole at ({}, {}) before the kept event {} (its "
						 "schedule is {})",
						 t, hole, lab.getPos(), schedule),
				     &g, ctx);
		}
	}
}

} // namespace invariants
