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


#ifndef GENMC_BACKWARD_REVISITS_HPP
#define GENMC_BACKWARD_REVISITS_HPP

/*
 * The backward revisits of a graph: what a revisit keeps (its view), and
 * which revisits the exploration takes (TruSt's maximality condition, which
 * makes every consistent graph reached by exactly one revisit: the
 * optimality of the exploration). Pure functions of the graph, the model's
 * prefix views and a few options: nothing here changes the graph or the
 * driver.
 */

#include "genmc/ADT/VectorClock.hpp"
#include "genmc/Support/Event.hpp"

#include <memory>

class BackwardRevisit;
class EventLabel;
class ExecutionGraph;
class GraphModel;
class MemAccessLabel;
class ReadLabel;
class WriteLabel;

/** The prefix view of LAB under MODEL (computed once, cached on the label) */
auto prefixView(const GraphModel &model, const EventLabel *lab) -> const VectorClock &;

/** The part of LAB's prefix a revisit by LAB keeps: the prefix, unless
 * SCOPESCTRLDEPS (LKMM with event keys), where the model narrows it */
auto revisitPrefixView(const GraphModel &model, bool scopesCtrlDeps, const EventLabel *lab)
	-> std::unique_ptr<VectorClock>;

class BackwardRevisits {
public:
	struct Options {
		/** The model tracks dependencies (IMM, LKMM) */
		bool depTracking = false;
		/** Barrier-aware model checking is off */
		bool disableBAM = false;
		/** See revisitPrefixView() */
		bool scopesCtrlDeps = false;
		/** -exhaustive (the default under -lkmm): a revisit view may be
		 * refused (see revisitView()) */
		bool exhaustive = false;
		/** -exhaustive under IMM: a revisit keeps the value dependencies
		 * of what it keeps (see keepValueDependencies()) */
		bool exhaustiveIMM = false;
	};

	BackwardRevisits(const ExecutionGraph &g, const GraphModel &model, Options opts)
		: g_(g), model_(model), opts_(opts)
	{}

	/** The revisit of RLAB by SLAB, with the view it keeps */
	[[nodiscard]] auto construct(const ReadLabel *rLab, const WriteLabel *sLab) const
		-> std::unique_ptr<BackwardRevisit>;

	/** What the revisit of RLAB by SLAB keeps: the events before RLAB and
	 * SLAB's prefix; null if no graph keeps it (-exhaustive under IMM, see
	 * keepValueDependencies(); -exhaustive otherwise, see keepRMWReads()) */
	[[nodiscard]] auto revisitView(const ReadLabel *rLab, const WriteLabel *sLab,
				       bool *keptRMWReads = nullptr) const
		-> std::unique_ptr<VectorClock>;

	/** Whether R, a backward revisit of the graph, is the one revisit that
	 * reaches its graph: the events it deletes were added maximally */
	[[nodiscard]] auto isMaximalExtension(const BackwardRevisit &r) const -> bool;

	/** Whether every read R keeps keeps its write (-exhaustive) */
	[[nodiscard]] auto isRevisitViewClosed(const BackwardRevisit &r) const -> bool;

	/** Whether R, by an RMW, keeps another RMW reading the same write */
	[[nodiscard]] auto keepsConflictingRMW(const BackwardRevisit &r) const -> bool;

private:
	/** Whether LAB, an access, is co-before a write of R's saved prefix
	 * (other than the revisiting one) */
	auto isCoBeforeSavedPrefix(const BackwardRevisit &r, const EventLabel *lab) const -> bool;

	/** Dependency-tracking models: whether LAB, an access the revisit R
	 * deletes (or the read it revisits), reads from or follows the co-latest
	 * write it can reach among the writes R keeps and those added before LAB */
	auto isPlacedMaximallyInRevisit(const BackwardRevisit &r, const MemAccessLabel *lab) const
		-> bool;

	/** Whether LAB is a write that a write of the saved prefix, added after LAB
	 * and to the same location, precedes in coherence */
	auto isCoAfterLaterSavedWrite(const BackwardRevisit &r, const EventLabel *lab) const
		-> bool;

	/** Whether the revisiting write's co-successor (if any) stays in the graph */
	auto coherenceSuccRemainInGraph(const BackwardRevisit &r) const -> bool;

	/** Whether LAB is in the prefix of a write that R keeps and that
	 * revisited a read still reading it */
	auto isKeptByKeptRevisit(const BackwardRevisit &r, const EventLabel *lab) const -> bool;

	/** -exhaustive under IMM: extends V with what the events it keeps
	 * depend on for their values; false if that needs the read at R or an
	 * event the graph does not have */
	auto keepValueDependencies(Event r, VectorClock &v) const -> bool;

	/** -exhaustive, without keepValueDependencies() (-lkmm): V extended
	 * so that every RMW write it keeps has its read (and what that reads
	 * from), without the write part of RLAB; null if that needs RLAB or a
	 * kept read reads RLAB's write part. KEPT (if given) tells whether
	 * a read was added */
	auto keepRMWReads(const ReadLabel *rLab, std::unique_ptr<VectorClock> v,
			  bool *kept) const -> std::unique_ptr<VectorClock>;

	auto getPrefixView(const EventLabel *lab) const -> const VectorClock &
	{
		return prefixView(model_, lab);
	}
	auto getRevisitPrefixView(const EventLabel *lab) const -> std::unique_ptr<VectorClock>
	{
		return revisitPrefixView(model_, opts_.scopesCtrlDeps, lab);
	}

	const ExecutionGraph &g_;
	const GraphModel &model_;
	Options opts_;
};

#endif /* GENMC_BACKWARD_REVISITS_HPP */
