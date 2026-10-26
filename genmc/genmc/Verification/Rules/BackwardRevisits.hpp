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
auto prefixView(const GraphModel &model, const EventLabel *lab, bool dependenciesOnly = false)
	-> const VectorClock &;

/** The part of LAB's prefix a revisit by LAB keeps: the prefix, unless
 * SCOPESCTRLDEPS (LKMM with event keys), where the model narrows it */
auto revisitPrefixView(const GraphModel &model, bool scopesCtrlDeps, const EventLabel *lab)
	-> std::unique_ptr<VectorClock>;

/** -exhaustive under -lkmm: the events LAB depends on, which a revisit by
 * LAB keeps. The closure of LAB under its dependencies (address, data,
 * control, the expected value of a CAS), rf, an RMW's read before its
 * write, and thread creation (a thread's start, its create, and the thread
 * of a join): what decides that LAB exists with its value, and nothing
 * ordered before it otherwise (fences, releases, same-location writes),
 * which the kat's ppo has. Those orderings depend on the path a branch
 * took: in the prefix, they could put the read a branch tests before a
 * write after its join, so that the write could not revisit it (or kept
 * the branch's events, which the new path did not reach); the exhaustive
 * exploration rechecks the whole graph when an event is added before kept
 * ones (see ExplorationCore::checkPendingKept()), so they need not be
 * kept (issue #77) */
auto dependencyPrefixView(const ExecutionGraph &g, const EventLabel *lab)
	-> std::unique_ptr<VectorClock>;

/** Where the events that depend on a read start from (see dependentsOf()) */
enum class DependentSeeds : unsigned char {
	Read,	  /**< the read itself */
	Scoped,	  /**< the read and the events inside a branch it controls */
	Branches, /**< the events inside a branch it controls, not the read */
};

/** The events that depend on RLAB, by stamp: the closure under the
 * dependents of an event (address, data, control, a CAS's expected value),
 * the readers of a write, an RMW's write after its read, and the threads
 * a create starts, of the SEEDS: RLAB, and with Scoped also the events that
 * exist because of what RLAB read (those inside a branch it controls, see
 * EventDeps::sctrl). A revisit of RLAB deletes the Scoped ones, and keeps
 * nothing because of them */
auto dependentsOf(const ExecutionGraph &g, const ReadLabel *rLab,
		  DependentSeeds seeds = DependentSeeds::Read) -> std::vector<char>;

/** -exhaustive under -lkmm: what a revisit of RLAB by SLAB keeps of SLAB's
 * prefix: the model's prefix (its revisit prefix, rppo and rf: a kept event
 * with its locks, fences and the same-location writes before it) without
 * the events that exist because of what RLAB read (the Scoped dependents,
 * see dependentsOf()), which RLAB's new path makes anew. Null if SLAB
 * depends on RLAB in any way, or RLAB is before SLAB whatever its value.
 *
 * The events inside a branch RLAB controls order things on their path only
 * (a fence, a release, a same-location write: the path-dependent part of
 * the prefix, issue #77): the prefix is computed as if they ordered
 * nothing, and RLAB may be in the model's prefix of SLAB only through
 * them. Only through them on its way to an event its thread keeps after
 * the branch, that is: when RLAB reaches SLAB through another thread's
 * read of a branch's event instead (another thread's lock taken after
 * RLAB's thread released it in the branch, say), that read can read
 * something else, and the graphs where it does have the revisit without
 * that path; the revisit is not taken (issue #261).
 *
 * Not so when RLAB is an RMW's read whose write is in the model's prefix
 * of SLAB (another thread's CAS reads it): the prefix is computed without
 * every dependent of RLAB, and RLAB may be in the model's prefix only
 * through them. The graphs where that CAS reads something else are
 * reached through a run that the RMWs make inconsistent, which the
 * deduplication of the exhaustive exploration may cut short (issue #264);
 * the revisit is taken */
auto keptPrefixView(const ExecutionGraph &g, const GraphModel &model, const ReadLabel *rLab,
		    const WriteLabel *sLab) -> std::unique_ptr<VectorClock>;

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
		/** -exhaustive under -lkmm: a revisit keeps what the write depends
		 * on (see dependencyPrefixView()) */
		bool keepsDependencies = false;
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

	/** Whether the read R revisits, reading the revisiting write, would go
	 * back in coherence against the first access to its location that R
	 * keeps after it in its thread */
	[[nodiscard]] auto breaksCoherenceOfKeptLater(const BackwardRevisit &r) const -> bool;

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
	 * from), without the write part of RLAB unless a kept read reads it
	 * and reading SLAB leaves its value as it is; null if that needs RLAB,
	 * or if a kept read reads a write part of RLAB that reading SLAB
	 * changes. KEPT (if given) tells whether a read was added */
	auto keepRMWReads(const ReadLabel *rLab, const WriteLabel *sLab,
			  std::unique_ptr<VectorClock> v, bool *kept) const
		-> std::unique_ptr<VectorClock>;

	auto getPrefixView(const EventLabel *lab) const -> const VectorClock &
	{
		return prefixView(model_, lab, opts_.keepsDependencies);
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
