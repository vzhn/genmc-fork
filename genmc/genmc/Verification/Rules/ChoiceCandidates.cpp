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

#include "genmc/Verification/Rules/ChoiceCandidates.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/GraphUtils.hpp"
#include "genmc/Verification/Rules/BackwardRevisits.hpp"

#include <algorithm>
#include <ranges>

/* The reads SLAB may revisit: same-location reads outside its prefix BEFORE.
 * TruSt skips the stable ones (in a revisit's prefix, or not reading
 * maximally: their revisits are explored from the maximal path), and the
 * reads added after an RMW that conflicts with SLAB. With ALL (-exhaustive),
 * every such read is a candidate, and BackwardRevisits::keepsConflictingRMW()
 * drops the revisits whose graph would violate atomicity */
static auto getRevisitable(WriteLabel *sLab, const VectorClock &before, bool all)
	-> std::vector<ReadLabel *>
{
	auto &g = *sLab->getParent();
	std::vector<ReadLabel *> loads;

	/* Helper function to erase loads in between conflicting RMWs */
	auto eraseConflictingLoads = [all](auto &sLab, auto &loads) {
		if (all)
			return;
		auto *confLab = findPendingRMW(sLab);
		if (!confLab)
			return;
		std::erase_if(loads,
			      [&](auto &eLab) { return eLab->getStamp() > confLab->getStamp(); });
	};
	auto candidate = [all, &before](const ReadLabel &rLab) {
		return (all || !rLab.isStable()) && !before.contains(rLab.getPos());
	};

	/* Fastpath: SLAB is co-maximal and the previous co-max is ppo-before
	 * it. Not for a write placed before others in coherence (a coherence
	 * revisit, see revisitWrite()): the readers of the writes co-before it
	 * can read from it too (#228: under IMM, a read filled into a hole
	 * before a kept write of its thread, which only SLAB placed co-before
	 * that write can revisit) */
	auto prevCoMaxIt = std::ranges::find_if(
		g.rco(sLab->getAddr()), [&](auto &lab) { return lab.getPos() != sLab->getPos(); });
	if (g.co_max(sLab->getAddr()) == sLab &&
	    prevCoMaxIt != std::ranges::end(g.rco(sLab->getAddr())) &&
	    before.contains(prevCoMaxIt->getPos())) {
		for (auto &rLab : prevCoMaxIt->readers()) {
			if (candidate(rLab))
				loads.push_back(&rLab);
		}
		eraseConflictingLoads(sLab, loads);
		return loads;
	}

	/* Slowpath: iterate over all same-location reads */
	for (auto it = ++ExecutionGraph::reverse_label_iterator(sLab);
	     it != ExecutionGraph::reverse_label_iterator(g.getInitLabel()); ++it) {
		auto *rLab = genmc::dyn_cast<ReadLabel>(&*it);
		if (rLab && rLab->getAddr() == sLab->getAddr() && candidate(*rLab))
			loads.push_back(rLab);
	}
	eraseConflictingLoads(sLab, loads);
	return loads;
}

/* Whether some read of ADDR is in G */
static auto hasReads(const ExecutionGraph &g, SAddr addr) -> bool
{
	const auto *iLab = g.getInitLabel();
	if (iLab->hasRfs(addr) && !std::ranges::empty(iLab->rfs(addr)))
		return true;
	return std::ranges::any_of(g.co(addr),
				   [](auto &wLab) { return !std::ranges::empty(wLab.readers()); });
}

/* -exhaustive under IMM. Whether a CAS succeeds decides its ordering (that of
 * its success or of its failure, see ReadLabel::setRf()) and whether it has
 * a write part. A CAS that succeeds can so be ordered before a write that
 * it would fail reading from (an acquire read before the write's events,
 * or its write part, a release one, before a write of its thread to the
 * location), and be in that write's prefix, where TruSt looks for the reads
 * the write cannot revisit. The default exploration relies on the CAS's
 * read being ordered before all later events of its thread whatever its
 * outcome, as IMM orders it (see Interpreter::visitAtomicCmpXchgInst()),
 * so that only its write part depends on the outcome, as under porf
 * (issue #137). The reference does not: such a CAS is revisitable all the
 * same when the write's value changes its ordering or makes it fail; the
 * revisit drops its write part (BackwardRevisits::revisitView()), and the
 * consistency of the graph it produces decides (backwardRevisit()).
 *
 * Whether reading from SLAB changes the ordering or the write of the CAS RLAB
 * (failsReading(): whether the CAS fails reading SLAB) */
static auto outcomeChanges(const ReadLabel *rLab, const WriteLabel *sLab) -> bool
{
	const auto *cLab = genmc::dyn_cast<CasReadLabel>(rLab);
	if (!cLab || !cLab->getRf())
		return false;
	const auto ord = cLab->valueMakesRMWSucceed(sLab->getAccessValue(cLab->getAccess()))
				 ? cLab->getSuccessOrdering()
				 : cLab->getFailOrdering();
	return ord != cLab->getOrdering() || failsReading(rLab, sLab);
}

/* -exhaustive under IMM: adds to LOADS the CAS reads in PREFIX, SLAB's
 * prefix, whose ordering or write part reading from SLAB would change */
static void addRevisitableCases(WriteLabel *sLab, const VectorClock &prefix,
				std::vector<ReadLabel *> &loads)
{
	for (auto &lab : sLab->getParent()->labels()) {
		auto *rLab = genmc::dyn_cast<ReadLabel>(&lab);
		if (rLab && rLab->getAddr() == sLab->getAddr() && prefix.contains(rLab->getPos()) &&
		    rLab->getThread() != sLab->getThread() && rLab->getRf() != sLab &&
		    outcomeChanges(rLab, sLab))
			loads.push_back(rLab);
	}
}

auto ChoiceCandidates::rfs(ReadLabel *lab) const -> std::vector<EventLabel *>
{
	auto &checker = model_;
	auto rfs = checker.getCoherentStores(lab);
	if (!genmc::isa<CasReadLabel>(lab) && !genmc::isa<FaiReadLabel>(lab))
		return rfs;

	/* Remove atomicity violations */
	const auto &before = prefixView(model_, lab);
	auto isSettledRMWInView = [&before](auto &rLab) {
		return rLab.isRMW() && (!rLab.isRevisitable() || before.contains(rLab.getPos()));
	};
	auto atomicityViolationInView = [&isSettledRMWInView, lab](auto *sLab) {
		if (auto *wLab = genmc::dyn_cast<WriteLabel>(sLab)) {
			return lab->valueMakesRMWSucceed(wLab->getAccessValue(wLab->getAccess())) &&
			       std::ranges::any_of(wLab->readers(), isSettledRMWInView);
		};

		auto *iLab = genmc::cast<InitLabel>(sLab);
		auto addr = lab->getAddr();
		/* Reads to dynamic addresses cannot have read from Init */
		return // !addr.isDynamic() &&
			lab->valueMakesRMWSucceed(iLab->getAccessValue(lab->getAccess())) &&
			std::ranges::any_of(iLab->rfs(addr), isSettledRMWInView);
	};
	std::erase_if(rfs, atomicityViolationInView);
	return rfs;
}

void ChoiceCandidates::dropAtomicityViolations(const ReadLabel *rLab,
					       std::vector<EventLabel *> &stores) const
{
	const auto &g = g_;
	if (!genmc::isa<CasReadLabel>(rLab) && !genmc::isa<FaiReadLabel>(rLab))
		return;

	const auto *casLab = genmc::dyn_cast<CasReadLabel>(rLab);
	auto valueMakesSuccessfulRMW = [&casLab](auto &&val) {
		return !casLab || val == casLab->getExpected();
	};
	std::erase_if(stores, [&](auto *sLab) {
		if (auto *iLab = genmc::dyn_cast<InitLabel>(sLab))
			return std::any_of(
				iLab->rf_begin(rLab->getAddr()), iLab->rf_end(rLab->getAddr()),
				[&](auto &rLab) {
					return rLab.isRMW() &&
					       valueMakesSuccessfulRMW(
						       rLab.getAccessValue(rLab.getAccess()));
				});
		return std::ranges::any_of(g.rf_succs(sLab), [&](auto &rLab) {
			return rLab.isRMW() &&
			       valueMakesSuccessfulRMW(rLab.getAccessValue(rLab.getAccess()));
		});
	});
}

auto ChoiceCandidates::revisitableReads(WriteLabel *sLab) const -> std::vector<ReadLabel *>
{
	/* Without a read of the location, there is nothing to revisit, and the
	 * prefix, costly under the dependency-tracking models, waits until it
	 * is needed (the other models' checkers take a read's prefix from its
	 * write's, so they keep computing it here) */
	if (opts_.depTracking && !hasReads(g_, sLab->getAddr()))
		return {};

	/* (Computed for the other models' checkers, see above) */
	prefixView(model_, sLab);
	auto prefix = revisitPrefixView(model_, opts_.scopesCtrlDeps, sLab);
	auto loads = getRevisitable(sLab, *prefix, opts_.exhaustive);
	model_.filterCoherentRevisits(sLab, loads);
	if (opts_.exhaustiveIMM)
		addRevisitableCases(sLab, *prefix, loads);
	std::ranges::sort(
		loads, [](auto &lab1, auto &lab2) { return lab1->getStamp() > lab2->getStamp(); });
	return loads;
}
