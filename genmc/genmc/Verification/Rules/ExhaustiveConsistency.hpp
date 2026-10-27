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

#ifndef GENMC_EXHAUSTIVE_CONSISTENCY_HPP
#define GENMC_EXHAUSTIVE_CONSISTENCY_HPP

/*
 * -exhaustive under IMM: the checks of a graph whose views are computed anew.
 * A revisit keeps events after holes with the views they had when they were
 * kept, so the views the exploration maintains can miss what the events that
 * fill the holes order; these checks compute the views from scratch, on the
 * graph or on a copy of it, and check the graph as a whole (coherence,
 * atomicity, the model's axioms). They are functions of the graph and the
 * model; how a label's views are updated is the caller's (it also updates the
 * prefixes of symmetry reduction, see ExplorationCore::updateLabelViews()).
 */

#include "genmc/Models/ConsistencyChecker.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"

#include <functional>
#include <utility>

class EventLabel;

class ExhaustiveConsistency {
public:
	using UpdateViews = std::function<void(EventLabel *)>;

	/** HBCOHERENCE: coherence is judged by the hb view (IMM: no cycle in
	 * hb ; eco); otherwise along each thread (LKMM: SC per location, the
	 * accesses of a thread to a location do not go back in its coherence
	 * order) */
	ExhaustiveConsistency(ConsistencyChecker &model, UpdateViews updateViews, bool hbCoherence)
		: model_(model), updateViews_(std::move(updateViews)), hbCoherence_(hbCoherence)
	{}

	/** Computes the views of G's labels anew, in G, and drops their cached
	 * prefix views. Returns false if they do not settle (an hb cycle) */
	auto recomputeViews(ExecutionGraph &g) -> bool;

	/** Drops the cached prefix views of G's labels: those of the events a
	 * revisit keeps without their prefix name events G does not have */
	static void dropPrefixViews(ExecutionGraph &g);

	/** Whether G, whose views are up to date, is consistent: coherence,
	 * the atomicity of its RMWs, the other axioms */
	auto isGraphConsistent(const ExecutionGraph &g, bool intermediate = false,
			       bool atomicity = true) const -> bool;

	/** Whether G (a complete execution, or the graph of an error) is
	 * consistent, checked on a copy whose views are computed anew; EXTRAS:
	 * with the axioms the exploration leaves out too (see
	 * ModelConsistency::satisfiesExtraAxioms()) */
	auto isExecutionConsistent(const ExecutionGraph &g, bool intermediate = false,
				   bool atomicity = true, bool extras = false) -> bool;

	/** Runs CHECK on a copy of G whose views are computed anew (positions
	 * name the same events in both) */
	void checkWithViewsAnew(const ExecutionGraph &g,
				const std::function<void(const ExecutionGraph &)> &check);

private:
	/** Whether G's accesses respect coherence along each thread */
	static auto isCoherentPerLocation(const ExecutionGraph &g) -> bool;
	auto isGraphConsistentImpl(const ExecutionGraph &g, bool atomicity) const -> bool;

	ConsistencyChecker &model_;
	UpdateViews updateViews_;
	bool hbCoherence_;
};

#endif /* GENMC_EXHAUSTIVE_CONSISTENCY_HPP */
