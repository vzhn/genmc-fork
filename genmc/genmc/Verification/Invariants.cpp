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

#include "genmc/Verification/Invariants.hpp"
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
#include "genmc/Verification/GraphPrinting.hpp"

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

/** Whether LAB, a pointer another label stores, is a label of G */
auto inGraph(const ExecutionGraph &g, const EventLabel *lab) -> bool
{
	return lab != nullptr && g.containsLab(lab);
}

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

} // namespace invariants
