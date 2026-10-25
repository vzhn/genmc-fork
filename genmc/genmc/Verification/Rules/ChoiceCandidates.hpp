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

#ifndef GENMC_CHOICE_CANDIDATES_HPP
#define GENMC_CHOICE_CANDIDATES_HPP

/*
 * The candidates a new event chooses among: the writes a read may read from,
 * and the reads a write may revisit. Each is a function of the graph and the
 * model (whose checker caches views, hence the non-const reference); trying
 * the candidates in the graph and pushing the alternatives is the core's (see
 * ExplorationCore::handleLoad() and ExplorationCore::calcRevisits()).
 */

#include "genmc/Models/ConsistencyChecker.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"

#include <vector>

class EventLabel;
class ReadLabel;
class WriteLabel;

class ChoiceCandidates {
public:
	struct Options {
		/** The model tracks dependencies (IMM, LKMM) */
		bool depTracking = false;
		/** See revisitPrefixView() */
		bool scopesCtrlDeps = false;
		/** -exhaustive */
		bool exhaustive = false;
		/** -exhaustive under IMM */
		bool exhaustiveIMM = false;
		/** -exhaustive under -lkmm: a write may revisit every read it
		 * does not depend on (see dependencyPrefixView()) */
		bool keepsDependencies = false;
	};

	ChoiceCandidates(const ExecutionGraph &g, ConsistencyChecker &model, Options opts)
		: g_(g), model_(model), opts_(opts)
	{}

	/** The writes RLAB may read from, ordered according to co: the coherent
	 * ones, without those an RMW settled in RLAB's prefix reads already
	 * (atomicity) */
	auto rfs(ReadLabel *rLab) const -> std::vector<EventLabel *>;

	/** Estimation and random runs: drops from STORES the writes an RMW
	 * reads already, which RLAB, an RMW, cannot read too */
	void dropAtomicityViolations(const ReadLabel *rLab,
				     std::vector<EventLabel *> &stores) const;

	/** The reads SLAB may revisit, in reverse-addition order */
	auto revisitableReads(WriteLabel *sLab) const -> std::vector<ReadLabel *>;

private:
	const ExecutionGraph &g_;
	ConsistencyChecker &model_;
	Options opts_;
};

#endif /* GENMC_CHOICE_CANDIDATES_HPP */
