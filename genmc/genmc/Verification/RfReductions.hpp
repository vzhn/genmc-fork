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


#ifndef GENMC_RF_REDUCTIONS_HPP
#define GENMC_RF_REDUCTIONS_HPP

/*
 * The reductions that drop writes a new read need not read from, because
 * another execution explored anyway covers that choice: symmetry reduction,
 * barrier-aware model checking (BAM), commuting blind RMWs, and SAVER's
 * value annotations. Each is a pure function of the graph and the read;
 * each is sound only under the conditions its comment states, which is
 * what a proof of the reduction has to establish.
 */

#include "genmc/Execution/Consistency/ConsistencyChecker.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"

#include <vector>

class EventLabel;
class ReadLabel;
class SymmetryChecker;
class WriteLabel;

/** The read part of WLAB if it is an RMW write, or null */
auto getRMWRead(const WriteLabel &wLab) -> const ReadLabel *;

/*
 * Decides which FAIs the blind-RMW reduction may reorder.  An FAI qualifies if
 * the interpreter marked it blind, it is relaxed, and it cannot be ordered
 * through synchronization: no release fence precedes it in its thread (which
 * would make its write a synchronization source) and no SC fence is in its hb
 * past (an SC fence hb-before one of two RMWs orders them through psc; one that
 * comes later can only follow both).  Built once per query, since it scans the
 * graph for SC fences.
 */
class BlindRMWFilter {
public:
	BlindRMWFilter(const ExecutionGraph &g, const GraphModel &cc) : g(g), cc(cc)
	{
		for (auto t = 0; t < g.getNumThreads(); t++) {
			auto first = -1;
			for (auto i = 0; first == -1 && i < g.getThreadSize(t); i++) {
				const auto *fLab =
					genmc::dyn_cast<FenceLabel>(g.getEventLabel(Event(t, i)));
				if (fLab && fLab->isSC())
					first = i;
			}
			firstSCFence.push_back(first);
		}
	}

	/* Returns LAB as a blind relaxed FAI that may be reordered, or null */
	auto asCommuting(const EventLabel *lab) const -> const FaiReadLabel *
	{
		const auto *fLab = genmc::dyn_cast_if_present<FaiReadLabel>(lab);
		if (!fLab || fLab->getKind() != EventLabel::FaiRead ||
		    fLab->getAttr() != WriteAttr::Blind ||
		    fLab->getOrdering() != MemOrdering::Relaxed)
			return nullptr;

		for (auto i = 0; i < fLab->getIndex(); i++) {
			const auto *pLab = genmc::dyn_cast<FenceLabel>(
				g.getEventLabel(Event(fLab->getThread(), i)));
			if (pLab && pLab->isAtLeastRelease())
				return nullptr;
		}
		const auto &hb = cc.getHbView(fLab);
		for (auto t = 0U; t < firstSCFence.size(); t++) {
			if (firstSCFence[t] != -1 &&
			    hb.getMax(static_cast<int>(t)) >= firstSCFence[t])
				return nullptr;
		}
		return fLab;
	}

	/* Whether A and B are distinct blind FAIs that can be reordered with each other */
	auto areTwins(const EventLabel *a, const EventLabel *b) const -> bool
	{
		const auto *fA = asCommuting(a);
		const auto *fB = asCommuting(b);
		return fA && fB && fA != fB && fA->getAddr() == fB->getAddr() &&
		       fA->getSize() == fB->getSize() && fA->getOp() == fB->getOp() &&
		       fA->getOpVal() == fB->getOpVal();
	}

private:
	const ExecutionGraph &g;
	const GraphModel &cc;
	std::vector<int> firstSCFence; /* -1 if the thread has none */
};

class RfReductions {
public:
	struct Options {
		bool symmetryReduction = false;
		bool disableBAM = false;
		bool blindRMWReduction = false;
		bool exhaustive = false;
	};

	RfReductions(const ExecutionGraph &g, const ConsistencyChecker &model,
		     const SymmetryChecker &symm, Options opts)
		: g_(g), model_(model), symm_(symm), opts_(opts)
	{}

	/** Drops from STORES, the coherent writes LAB may read from, those
	 * the enabled reductions cover by another execution */
	void filter(const ReadLabel *lab, std::vector<EventLabel *> &stores) const;

private:
	/** Symmetry reduction: a write the symmetric predecessor thread's
	 * matching RMW already reads from */
	void filterSymmetricStoresSR(const ReadLabel *rLab, std::vector<EventLabel *> &stores) const;

	/** BAM: a barrier's wait reads the round's maximal increment; its
	 * increment reads no write an exclusive read reads */
	void filterConflictingBarriers(const ReadLabel *lab, std::vector<EventLabel *> &stores) const;

	/** Identical blind RMWs commute (see the definition) */
	void filterCommutingBlindRMWs(const ReadLabel *lab, std::vector<EventLabel *> &stores) const;

	/** SAVER: a value the read's annotation rejects (the read would block) */
	void filterValuesFromAnnotSAVER(const ReadLabel *rLab,
					std::vector<EventLabel *> &stores) const;

	const ExecutionGraph &g_;
	const ConsistencyChecker &model_;
	const SymmetryChecker &symm_;
	Options opts_;
};

#endif /* GENMC_RF_REDUCTIONS_HPP */
