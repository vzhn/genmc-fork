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

#include "GenMCDriver.hpp"
#include "genmc/ADT/DepView.hpp"
#include "genmc/Models/BoundDecider.hpp"
#include "genmc/Models/ConsistencyChecker.hpp"
#include "genmc/Models/SymmetryChecker.hpp"
#include "genmc/Execution/DepExecutionGraph.hpp"
#include "genmc/Execution/DepInfo.hpp"
#include "genmc/Execution/EventAttr.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionState.hpp"
#include "genmc/Execution/GraphUtils.hpp"
#include "genmc/Models/LkmmModel.hpp"
#include "genmc/Execution/LoadAnnotation.hpp"
#include "genmc/Execution/Stamp.hpp"
#include "genmc/Support/ASize.hpp"
#include "genmc/Support/ActionEnums.hpp"
#include "genmc/Support/Cast.hpp"
#include "genmc/Support/Error.hpp"
#include "genmc/Support/Logger.hpp"
#include "genmc/Support/MemAccess.hpp"
#include "genmc/Support/MemOrdering.hpp"
#include "genmc/Support/ModuleVarID.hpp"
#include "genmc/Support/NameInfo.hpp"
#include "genmc/Support/SExprVisitor.hpp"
#include "genmc/Support/ThreadInfo.hpp"
#include "genmc/Support/Verbosity.hpp"
#include "genmc/Verification/Config.hpp"
#include "genmc/Verification/Core/DriverHandlerDispatcher.hpp"
#include "genmc/Verification/Rules/ErrorRules.hpp"
#include "genmc/Verification/Report/GraphPrinting.hpp"
#include "genmc/Verification/Checks/Invariants.hpp"
#include "genmc/Verification/Relinche/LinearizabilityChecker.hpp"
#include "genmc/Verification/Relinche/Specification.hpp"
#include "genmc/Verification/State/Revisit.hpp"
#include "genmc/Verification/State/Scheduler.hpp"
#include "genmc/Verification/Rules/SymmetricThreads.hpp"
#include "genmc/Support/VerificationError.hpp"
#include "genmc/Verification/VerificationResult.hpp"
#include "genmc/Verification/State/WorkList.hpp"
#include "genmc/config.h"

#include <algorithm>
#include <cstdio>
#include <chrono>
#include <climits>
#include <cstdint>
#include <format>
#include <memory>
#include <numeric>
#include <optional>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace std::string_literals;

/************************************************************
 ** GENERIC MODEL CHECKING DRIVER
 ***********************************************************/

GenMCDriver::~GenMCDriver() = default;

static void reconstructState(EventLabel *eLab, View &view, ExecutionState &state)
{
	if (!eLab || view.contains(eLab->getPos()))
		return;

	auto i = view.getMax(eLab->getThread());
	view.updateIdx(eLab->getPos());

	auto &g = *eLab->getParent();
	for (++i; i <= eLab->getIndex(); i++) {
		const auto *lab = g.getEventLabel(Event(eLab->getThread(), i));
		if (const auto *rLab = genmc::dyn_cast<ReadLabel>(lab))
			reconstructState(rLab->getRf(), view, state);
		if (const auto *jLab = genmc::dyn_cast<ThreadJoinLabel>(lab))
			reconstructState(g.getLastThreadLabel(jLab->getChildId()), view, state);
		if (const auto *bLab = genmc::dyn_cast<ThreadStartLabel>(lab)) {
			reconstructState(bLab->getCreate(), view, state);
		}

		if (const auto *rLab = genmc::dyn_cast<ReadLabel>(lab)) {
			if (rLab->isNotAtomic()) {
				g.getState().onNALoad(
					rLab->getPos(), rLab->getAccess(),
					eLab->getParent()->getMemModel()->getHbView(rLab));
			} else {
				/* TODO: If we support emitNA + lazy NA value, we should notify
				 * the corresponding value here */
				g.getState().onATLoad(rLab->getPos(), rLab->getAccess(),
						      g.getMemModel()->getHbView(rLab));
			}
		} else if (const auto *wLab = genmc::dyn_cast<WriteLabel>(lab)) {
			if (wLab->isNotAtomic())
				g.getState().onNAStore(wLab->getPos(), wLab->getAccess(),
						       g.getMemModel()->getHbView(wLab),
						       wLab->getVal());
			else {
				/* TODO: If we support emitNA + lazy NA value, we should notify
				 * the corresponding value here */
				g.getState().onATStore(wLab->getPos(), wLab->getAccess(),
						       g.getMemModel()->getHbView(wLab),
						       wLab->isEffectful(),
						       wLab->isAtLeastRelease(), getLockRole(wLab));
			}
		} else if (const auto *aLab = genmc::dyn_cast<MallocLabel>(lab)) {
			/* Keep the address the graph has: under dependency-tracking
			 * models, a thread's earlier allocations may have been cut
			 * from the graph, and are allocated anew after this one */
			state.onAllocAt(aLab->getPos(), aLab->getAddr(), aLab->getSize(),
					aLab->getAlignment());
			if (aLab->isZeroed())
				state.onZeroedAlloc(aLab->getPos(), aLab->getAccess(),
						    g.getMemModel()->getHbView(aLab));
		} else if (const auto *dLab = genmc::dyn_cast<FreeLabel>(lab)) {
			/* An invalid free (an error event) frees no block */
			auto addr = dLab->getAddr();
			if (state.isAllocated(addr) && state.getAllocAccess(addr).addr == addr)
				state.onFree(dLab->getPos(), addr, genmc::isa<HpRetireLabel>(dLab));
		} else if (const auto *ssLab = genmc::dyn_cast<SpinStartLabel>(lab)) {
			state.onSpinStart(ssLab->getPos(), ssLab->getLoop());
		} else if (const auto *lbLab = genmc::dyn_cast<LoopBeginLabel>(lab)) {
			state.onLoopBegin(lbLab->getPos(), lbLab->getLoop());
		} else if (genmc::isa<ThreadCreateLabel>(lab)) {
			state.onThreadCreate(lab->getPos());
		} else if (const auto *asLab = genmc::dyn_cast<AwaitStartLabel>(lab)) {
			state.onAwaitStart(asLab->getPos());
		} else if (const auto *hpLab = genmc::dyn_cast<HpProtectLabel>(lab)) {
			state.onProtect(hpLab->getHpAddr(), hpLab->getProtectedAddr());
		}
	}
}

static void reconstructState(ExecutionGraph &g, ExecutionState &state)
{
	View view;
	for (auto i = 1; std::cmp_less(i, g.getNumThreads()); i++)
		view.setMax(Event(i, -1));
	for (auto i = 0; i < g.getNumThreads(); i++)
		reconstructState(g.getLastThreadLabel(i), view, state);
}

auto GenMCDriver::handleExecutionStart() -> bool
{
	auto &g = getExec().getGraph();
	auto &state = getExec().getGraph().getState();

	/* Set various exploration options for this execution */
	unmoot();
	g.setKeptEventsUnreachable(false);
	keptRuns_.startRun(g.getMaxStamp(), getExec().getWorkqueue().size());
	state.clear();
	if constexpr (Config::emitNALabels)
		reconstructState(g, state);
	if constexpr (Config::emitNALabels)
		checkAllocationInvariants("reconstructing the state");
	/* An error replay replays to its target, which the graph holds (I2) */
	if (checksInvariants() && getScheduler().inErrorReplay()) {
		auto target = *getScheduler().getErrorReplayEvent();
		if (!errorReplay_->buffered() || !g.containsPosNonEmpty(target))
			invariants::fail("I2 (error replay)",
					 std::format("an execution starts in error replay but its "
						     "target {} is {} (buffered error: {})",
						     target,
						     g.containsPosNonEmpty(target)
							     ? "in the graph"
							     : "not in the graph",
						     errorReplay_->buffered().has_value()),
					 &g, invariantContext("starting an execution"));
	}
	getScheduler().resetExplorationOptions(g);
	/* A resumed execution is driven through the holes before its kept
	 * events as a replayed one is (I4) */
	if (resumePoints_.pending() && checksInvariants())
		invariants::checkResumeSchedule(g, getScheduler().getReplaySchedule(),
						invariantContext("starting a resumed execution"));
	return getScheduler().inErrorReplay();
}

void GenMCDriver::checkHelpingCasAnnotation()
{
	/* If we were waiting for a helped CAS that did not appear, complain */
	auto &g = getExec().getGraph();
	for (auto i = 0; i < g.getNumThreads(); i++) {
		if (genmc::isa<HelpedCASBlockLabel>(g.getLastThreadLabel(i)))
			ERROR("Helped/Helping CAS annotation error! Does helped CAS always "
			      "execute?");
	}

	/* Next, we need to check whether there are any extraneous
	 * stores, not visible to the helped/helping CAS */
	for (auto &lab : g.labels() | std::views::filter([](auto &lab) {
				 return genmc::isa<HelpingCasLabel>(&lab);
			 })) {
		auto *hLab = genmc::dyn_cast<HelpingCasLabel>(&lab);

		/* Check that all stores that would make this helping
		 * CAS succeed are read by a helped CAS.
		 * We don't need to check the swap value of the helped CAS */
		if (std::ranges::any_of(g.co(hLab->getAddr()), [&](auto &sLab) {
			    return hLab->getExpected() == sLab.getVal() &&
				   std::ranges::none_of(sLab.readers(), [&](auto &rLab) {
					   return genmc::isa<HelpedCasReadLabel>(&rLab);
				   });
		    }))
			ERROR("Helped/Helping CAS annotation error! "
			      "Unordered store to helping CAS location!");

		/* Special case for the initializer (as above) */
		if (hLab->getAddr().isStatic() &&
		    hLab->getExpected() == g.getInitVal(hLab->getAccess())) {
			auto rsView = g.labels() | std::views::filter([hLab](auto &lab) {
					      auto *rLab = genmc::dyn_cast<ReadLabel>(&lab);
					      return rLab && rLab->getAddr() == hLab->getAddr();
				      });
			if (std::ranges::none_of(rsView, [&](auto &lab) {
				    return genmc::isa<HelpedCasReadLabel>(&lab);
			    }))
				ERROR("Helped/Helping CAS annotation error! "
				      "Unordered store to helping CAS location!");
		}
	}
}

#ifdef ENABLE_GENMC_DEBUG
void GenMCDriver::trackExecutionBound()
{
	auto bound = bounder->calculate(getExec().getGraph());
	result.exploredBounds.grow(bound);
	result.exploredBounds[bound]++;
}
#endif

void GenMCDriver::updateStSpaceEstimation()
{
	/* Calculate current sample */
	auto &choices = getExec().getChoiceMap();
	auto sample = std::accumulate(
		choices.begin(), choices.end(),
		1.0L, // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
		[](auto sum, auto &kv) { return sum *= kv.second.size(); });

	/* This is the (i+1)-th exploration */
	auto totalExplored = (long double)result.explored + result.exploredBlocked + 1L;

	/* As the estimation might stop dynamically, we can't just
	 * normalize over the max samples to avoid overflows. Instead,
	 * use Welford's online algorithm to calculate mean and
	 * variance. */
	auto prevM = result.estimationMean;
	auto prevV = result.estimationVariance;
	result.estimationMean += (sample - prevM) / totalExplored;
	result.estimationVariance +=
		((sample - prevM) / totalExplored * (sample - result.estimationMean)) -
		(prevV / totalExplored);
}

static const auto maybeTimeRelinche = [](auto &&relinche, auto &&g) {
#ifdef ENABLE_GENMC_DEBUG
	const auto &start = std::chrono::high_resolution_clock::now();
#endif
	auto res = relinche.refinesSpec(g);
#ifdef ENABLE_GENMC_DEBUG
	const auto &stop = std::chrono::high_resolution_clock::now();
	res.analysisTime = stop - start;
#endif
	return res;
};

auto GenMCDriver::handleExecutionEnd() -> std::optional<VerificationError>
{
	auto &g = getExec().getGraph();

	/* An error replay that ended without reaching its target: report the
	 * error with the metadata collected so far rather than lose it (or carry
	 * the replay into the next execution) */
	if (errorReplay_->active() && errorReplay_->buffered()) {
		WARN_ONCE("incomplete-error-replay",
			  "The error report may lack source information (incomplete replay).\n");
		auto details = *errorReplay_->buffered();
		reportError(*errorReplay_->target(), details);
		return details.shouldHalt ? std::optional(details.type) : std::nullopt;
	}

	if (isMoot() || !checkPendingKept()) {
		GENMC_DEBUG(++result.exploredMoot;);
		return {};
	}

	/* A thread stopped before events that others observe: not an execution
	 * (whether blocked or not), see alignKeptEvents() */
	if (relocatesKeptEvents() && lkmm::hasThreadBlockedBeforeKept(g)) {
		++result.relocationsLost;
		return {};
	}

	/* Helper: Check helping CAS annotation */
	if (getConf()->helper)
		checkHelpingCasAnnotation();

	/* If in estimation mode, guess the total.
	 * (This may run a few times, but that's OK.)*/
	if (inEstimationMode()) {
		updateStSpaceEstimation();
		if (!shouldStopEstimating())
			getExec().getWorkqueue().add(std::make_unique<RerunForwardRevisit>());
	}

	/* Ignore the execution if some assume has failed */
	if (g.isBlocked()) {
		++result.exploredBlocked;
		if (std::ranges::any_of(g.thr_ids(), [&](auto tid) {
			    const auto *lab = g.getLastThreadLabel(tid);
			    return genmc::isa<SpinloopBlockLabel>(lab) ||
				   genmc::isa<AwaitBlockLabel>(lab);
		    }))
			++result.spinBlocked;
		if (getConf()->printBlockedExecs)
			printGraph(g, errorReplay_->dbgInfo());
		if (getConf()->checkLiveness)
			return checkLiveness();
		return {};
	}

	if (getConf()->warnUnfreedMemory)
		if (const auto err = checkUnfreedMemory())
			return err;
	if (const auto err = checkLkmmSrcu())
		return err;
	if (const auto err = checkLkmmRace())
		return err;

	/* -exhaustive under IMM: the exploration checks an event that fills a
	 * hole (one the threads add before events a revisit kept) as if it were
	 * the last event of its thread (rf and coherence choices among the
	 * coherent ones, and the extensibility of the last choice), and the
	 * views of the kept events after it are those they had when they were
	 * kept: the complete execution is counted only if it is consistent when
	 * checked anew (on a copy, so that the exploration from the graph is not
	 * changed) */
	if (getConf()->exhaustive && getConf()->model == ModelType::IMM &&
	    !isExecutionConsistent(g)) {
		++result.exploredInconsistent;
		return {};
	}

	/* -exhaustive reaches an execution by every path to it: count each once */
	if (getConf()->exhaustive && !countedExecutions_.insert(canonicalGraphHash(g)).second) {
		++result.exploredDuplicates;
		return {};
	}
	if (getConf()->printExecGraphs)
		printGraph(g, errorReplay_->dbgInfo());

	GENMC_DEBUG(if (getConf()->boundsHistogram && inVerificationMode()) trackExecutionBound(););

	++result.explored;
	if (fullExecutionExceedsBound())
		++result.boundExceeding;

	if (isHalting() || g.isBlocked() || isMoot())
		return {};

	if (!inVerificationMode())
		return {};

	/* TODO: figure out how to add linearization to random mode */
	/* Relinche: Collect/check abstract behavior */
	if (getConf()->collectLinSpec)
		result.specification->add(g, &getConsChecker(), getConf()->symmetryReduction);
	if (getConf()->checkLinSpec) {
		result.relincheResult += maybeTimeRelinche(getRelinche(), getExec().getGraph());
		if (result.relincheResult.status) {
			result.status = VerificationError::VE_LinearizabilityError;
			reportError(std::ranges::begin(g.rlabels())->getPos(),
				    {std::nullopt, *result.status,
				     result.relincheResult.status->toString()});
			return {VerificationError::VE_LinearizabilityError};
		}
	}
	return {};
}

auto GenMCDriver::scheduleNext(std::span<Action> runnable) -> ScheduleResult
{
	auto &g = getExec().getGraph();
	if (keptRuns_.hasAddedBeforeKept() && !isMoot()) {
		keptRuns_.takeAddedBeforeKept();
		if (!keptRuns_.eagerChecks())
			keptRuns_.deferCheck();
		else if (!getConsChecker().isConsistent(g))
			moot();
	}
	if (g.keptEventsUnreachable()) {
		g.setKeptEventsUnreachable(false);
		++result.relocationsLost;
		moot();
	}
	if (isHalting())
		return Error{};
	if (isMoot())
		return Blocked{};

	if (auto next = getScheduler().schedule(g, runnable); next) {
		if (auto r = getScheduler().takeReadToUnblock())
			unblockThread(g, *r);
		return *next;
	}
	return g.isBlocked() ? ScheduleResult(Blocked{}) : ScheduleResult(Finished{});
}

auto GenMCDriver::runFromCache() -> bool
{
	if (!getConf()->instructionCaching || !inVerificationMode())
		return false;

	do {
		auto &g = getExec().getGraph();
		auto next = getScheduler().nextThreadFromCache(g);
		if (!next)
			return true;
		if (auto r = getScheduler().takeReadToUnblock())
			unblockThread(g, *r);
		auto *toAdd = getScheduler().retrieveFromCache(g, *next);
		if (toAdd == nullptr)
			return false;

		addLabelsToGraph(*toAdd);
	} while (!isMoot() && !isHalting());
	return true;
}

auto GenMCDriver::fullExecutionExceedsBound() const -> bool
{
	return executionExceedsBound(BoundCalculationStrategy::NonSlacked);
}

void GenMCDriver::addLabelsToGraph(const std::vector<std::unique_ptr<EventLabel>> &labs)
{
	auto &g = getExec().getGraph();
	DriverHandlerDispatcher dispatcher(this);

	for (const auto &vlab : labs) {
		VERIFY(!vlab->hasStamp());

		if (!isExecutionDrivenByGraph(vlab->getPos()))
			dispatcher.visit(vlab);

		if (isMoot() || genmc::isa<BlockLabel>(g.getLastThreadLabel(vlab->getThread())))
			break;
	}

	/* Graph well-formedness: ensure RMWs events are scheduled as one.
	 * (Cannot rely on next round scheduling the same thread.) */
	auto *lastLab = &*getExec().getGraph().rlabels().begin();
	if (auto *rLab = genmc::dyn_cast<ReadLabel>(lastLab)) {
		if (auto wLab = createRMWWriteLabel(g, rLab))
			dispatcher.visit(*wLab);
	}
}

auto GenMCDriver::toCasResult(HandleResult<SVal> res, Event pos) -> HandleResult<CasReadResult>
{
	if (auto *val = std::get_if<SVal>(&res.result)) {
		const auto *cLab =
			genmc::cast<CasReadLabel>(getExec().getGraph().getEventLabel(pos));
		auto success = cLab->valueMakesRMWSucceed(*val) && !cLab->failsSpuriously();
		return {.result = CasReadResult{.val = *val, .success = success},
			.count = res.count};
	}
	if (auto *err = std::get_if<VerificationError>(&res.result))
		return {.result = *err, .count = res.count};
	if (std::holds_alternative<Reset>(res.result))
		return {.result = Reset{}, .count = res.count};
	return {.result = Invalid{}, .count = res.count};
}

static auto threadReadsMaximal(const ExecutionGraph &g, int tid) -> bool
{
	/* Depending on whether this is a DSA loop or not, we have to
	 * adjust the detection starting point: DSA-blocked threads
	 * will have a SpinStart as their last event */
	VERIFY(genmc::isa<BlockLabel>(g.getLastThreadLabel(tid)));
	/* A blocked await is judged by the reads of its condition (back to its
	 * AwaitStart). A blocked spinloop is judged by its whole iteration (back
	 * to the SpinStart), including the reads of any await nested in it.
	 * The iteration of a loop includes the loops nested in it: a blocked
	 * SpinStart judges the iteration since the previous SpinStart of its
	 * loop. (A spin end has no spinloop nested in its iteration, which
	 * starts at the last SpinStart; see SpinAssumePass.) */
	const auto *bLab = g.getLastThreadLabel(tid);
	const auto isAwait = genmc::isa<AwaitBlockLabel>(bLab);
	const auto *lastLab = g.po_imm_pred(bLab);
	if (isAwait && genmc::isa<AwaitStartLabel>(lastLab))
		return true;
	const auto *ssLab = genmc::dyn_cast<SpinStartLabel>(lastLab);
	auto start = ssLab ? lastLab->getPos().prev() : lastLab->getPos();
	std::optional<LoopKey> loop;
	if (const auto *sbLab = genmc::dyn_cast<SpinloopBlockLabel>(bLab))
		loop = sbLab->getLoop();
	if (!loop && ssLab)
		loop = ssLab->getLoop();
	auto isIterStart = [&](const EventLabel *lab) {
		if (isAwait)
			return genmc::isa<AwaitStartLabel>(lab);
		const auto *sLab = genmc::dyn_cast<SpinStartLabel>(lab);
		return sLab && (!loop || sLab->getLoop() == *loop);
	};

	/* Helper to get the co-max label */
	auto getCoMax = [&](auto &rLab) {
		/* Exclude RMW successor for ZNE spinloops */
		auto rco = g.rco(rLab->getAddr());
		auto coIt = std::ranges::find_if(rco, [&](auto &sLab) {
			/* (A write that had nothing to revisit may lack its prefix) */
			if (!sLab.hasPrefixView())
				sLab.setPrefixView(g.getMemModel()->calculatePrefixView(&sLab));
			return !sLab.getPrefixView().contains(rLab->getPos());
		});
		return coIt == std::ranges::end(rco) ? g.getInitLabel()
						     : (const EventLabel *)&*coIt;
	};

	/* A failed lock acquisition or condition-variable wait blocks on its
	 * read alone, with no loop markers around it. (A successful one inside
	 * a spinloop may also end the iteration: that is judged below.) */
	if (const auto *rLab = genmc::dyn_cast<ReadLabel>(lastLab);
	    rLab && (genmc::isa<LockCasReadLabel>(rLab) || genmc::isa<CondVarWaitReadLabel>(rLab)) &&
	    rLab->getAnnot() &&
	    !rLab->valueMakesAssumeSucceed(rLab->getAccessValue(rLab->getAccess()))) {
		const auto *coMax = getCoMax(rLab);
		if (rLab->getRf() != coMax)
			return false;
		/* A lock ZNE spinloop is blocked between its lock and its
		 * unlock: if the lock was taken in that blocked iteration, its
		 * holder releases it */
		const auto *hLast = g.getLastThreadLabel(coMax->getThread());
		if (!genmc::isa<LockZNEBlockLabel>(hLast))
			return true;
		for (const auto *lab = g.po_imm_pred(hLast); lab; lab = g.po_imm_pred(lab)) {
			if (genmc::isa<SpinStartLabel>(lab))
				return !(lab->getPos() < coMax->getPos());
		}
		return true;
	}

	/* An iteration that released a mutex can be observed: another thread
	 * may take the mutex after it and write what the next iteration reads,
	 * although that write depends on the iteration. Then only the thread's
	 * own later writes are out of reach of the next iteration */
	auto observable = false;
	for (auto j = start.index; j > 0; j--) {
		const auto *lab = g.getEventLabel(Event(tid, j));
		if (isIterStart(lab))
			break;
		observable |= genmc::isa<UnlockWriteLabel>(lab);
	}
	auto getReachableCoMax = [&](auto &rLab) -> const EventLabel * {
		if (!observable)
			return getCoMax(rLab);
		auto rco = g.rco(rLab->getAddr());
		auto coIt = std::ranges::find_if(rco, [&](auto &sLab) {
			return sLab.getThread() != tid || sLab.getIndex() < rLab->getIndex();
		});
		return coIt == std::ranges::end(rco) ? g.getInitLabel()
						     : (const EventLabel *)&*coIt;
	};

	for (auto j = start.index; j > 0; j--) {
		const auto *lab = g.getEventLabel(Event(tid, j));
		if (isIterStart(lab))
			return true;
		VERIFY(!genmc::isa<LoopBeginLabel>(lab) ||
		       (loop && genmc::cast<LoopBeginLabel>(lab)->getLoop() != *loop));
		if (const auto *rLab = genmc::dyn_cast<ReadLabel>(lab)) {
			if (rLab->getRf() != getReachableCoMax(rLab))
				return false;
		}
	}
	UNREACHABLE();
}

auto GenMCDriver::checkLiveness() -> std::optional<VerificationError>
{
	if (isHalting())
		return {};

	const auto &g = getExec().getGraph();

	/* A failed assume makes the whole execution infeasible: threads may
	 * wait forever on the thread it cut off */
	if (std::ranges::any_of(g.thr_ids(), [&](auto tid) {
		    return genmc::isa<UserBlockLabel>(g.getLastThreadLabel(tid));
	    }))
		return {};

	/* Collect all threads blocked at spinloops or awaits */
	std::vector<int> spinBlocked;
	for (auto i = 0; i < g.getNumThreads(); i++) {
		const auto *lab = g.getLastThreadLabel(i);
		if (genmc::isa<SpinloopBlockLabel>(lab) || genmc::isa<AwaitBlockLabel>(lab))
			spinBlocked.push_back(i);
	}

	if (spinBlocked.empty())
		return {};

	/* And check whether all of them are live or not */
	auto nonTermTID = 0;
	if (std::ranges::all_of(spinBlocked, [&](int tid) {
		    nonTermTID = tid;
		    return threadReadsMaximal(g, tid);
	    })) {
		/* Print some TID blocked by a spinloop or an await */
		auto lastPos = g.getLastThreadLabel(nonTermTID)->getPos();
		auto what = genmc::isa<AwaitBlockLabel>(g.getLastThreadLabel(nonTermTID))
				    ? "Non-terminating wait: thread "
				    : "Non-terminating spinloop: thread ";
		reportError(lastPos, {lastPos, VerificationError::VE_Liveness,
				      what + std::to_string(nonTermTID)});
		return {VerificationError::VE_Liveness};
	}
	return {};
}

auto GenMCDriver::checkLkmmRace() -> std::optional<VerificationError>
{
	/* LKMM's races are a property of a whole execution: a pair can be
	 * ordered by events added after both (pb, rb), unlike hb under C11 */
	if (getConf()->model != ModelType::LKMM || getConf()->disableRaceDetection ||
	    inEstimationMode() || isHalting())
		return {};

	auto race = lkmm::findRace(getExec().getGraph());
	if (!race)
		return {};
	/* Reported at the access added later */
	const auto *first = race->a;
	const auto *second = race->b;
	if (second->getStamp() < first->getStamp())
		std::swap(first, second);
	reportError(second->getPos(), {second->getPos(), VerificationError::VE_RaceNotAtomic,
				       "The accesses are a data race of LKMM (" + race->kind +
					       ").\n",
				       first});
	return {VerificationError::VE_RaceNotAtomic};
}

auto GenMCDriver::checkLkmmSrcu() -> std::optional<VerificationError>
{
	/* The matching of an srcu_read_unlock() (srcu-rscs) can go through
	 * other threads (srcu_down_read() and srcu_up_read()): decided on a
	 * whole execution */
	if (getConf()->model != ModelType::LKMM || inEstimationMode() || isHalting())
		return {};

	auto &g = getExec().getGraph();
	if (auto misuse = lkmm::findSrcuMisuse(g)) {
		auto pos = misuse->lab->getPos();
		reportError(pos, {pos, VerificationError::VE_InvalidRcu, misuse->what + "!",
				  misuse->other});
		return {VerificationError::VE_InvalidRcu};
	}

	/* An access was checked against a free when added, with a critical
	 * section still open taken as closed at the end of its thread; one
	 * that srcu_up_read() closed in another thread may end earlier, so
	 * check again with every section matched */
	auto pairs = lkmm::srcuRscs(g);
	if (std::ranges::none_of(pairs, [](auto &p) {
		    return p.first->getThread() != p.second->getThread();
	    }))
		return {};
	auto unordered = lkmm::findUnorderedFree(g, [&](auto *a, auto *f) {
		return g.getMemModel()->getHbView(f).contains(a->getPos());
	});
	if (!unordered)
		return {};
	auto [a, fLab] = *unordered;
	reportError(a->getPos(), {a->getPos(), VerificationError::VE_AccessFreed, "", fLab});
	return {VerificationError::VE_AccessFreed};
}

auto GenMCDriver::checkUnfreedMemory() -> std::optional<VerificationError>
{
	if (isHalting())
		return {};

	auto &g = getExec().getGraph();
	Event unfreedAlloc{};
	if (std::ranges::any_of(g.getState().allocations(), [&](const auto &kv) {
		    unfreedAlloc = kv.second.pos;
		    return !kv.second.freePos.has_value();
	    })) {
		/* Return an error only if the warning was upgraded to an error */
		if (reportWarningOnce(unfreedAlloc, VerificationError::VE_UnfreedMemory))
			return {VerificationError::VE_UnfreedMemory};
	}
	return {};
}

auto GenMCDriver::handleThreadKill(const EventDbgInfo *dbg, Event pos)
	-> HandleResult<std::monostate>
{
	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos)) {
		/* The kill can be reached while driven by the graph: by an error replay
		 * that targets it (updateErrorInfoAndMaybeExit() may have just ended the
		 * replay), or by a replay with race detection disabled */
		VERIFY(genmc::isa<ThreadKillLabel>(getExec().getGraph().getEventLabel(pos)));
		/* (I2: an ongoing error replay only re-executes the kill if it is
		 * its target, or if non-atomic events stay in its schedule) */
		if (checksInvariants() && getScheduler().inErrorReplay() &&
		    !getScheduler().isErrorReplayEvent(pos) && !getConf()->disableRaceDetection &&
		    !getConf()->replayCompletedThreads)
			invariants::fail(
				"I2 (error replay)",
				std::format("the kill {} is re-executed by an error replay "
					    "targeting {}",
					    pos, *getScheduler().getErrorReplayEvent()),
				&getExec().getGraph(), invariantContext("replaying a kill"));
		return {.result = std::monostate{}, .count = 1U};
	}
	return handleThreadKill(ThreadKillLabel::create(pos));
}

auto GenMCDriver::handleThreadCreate(const EventDbgInfo *dbg, Event pos, const ThreadInfo &info,
				     const EventDeps &deps) -> HandleResult<int>
{
	auto &g = getExec().getGraph();

	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos)) {
		if constexpr (!Config::emitNALabels)
			g.getState().onThreadCreate(pos);
		return {.result = genmc::dyn_cast<ThreadCreateLabel>(g.getEventLabel(pos))
					  ->getChildId(),
			.count = 1U};
	}
	return handleThreadCreate(ThreadCreateLabel::create(pos, info, deps));
}

auto GenMCDriver::handleThreadJoin(const EventDbgInfo *dbg, Event pos, int childTid,
				   const EventDeps &deps, bool resultUsed) -> HandleResult<SVal>
{
	auto &g = getExec().getGraph();

	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos))
		return {.result = g.getEventLabel(pos)->getReturnValue(), .count = 1U};
	auto lab = ThreadJoinLabel::create(pos, childTid, deps);
	lab->setResultUsed(resultUsed);
	return handleThreadJoin(std::move(lab));
}

auto GenMCDriver::handleThreadFinish(const EventDbgInfo *dbg, Event pos, SVal val,
				     const EventDeps &deps)
	-> HandleResult<std::monostate>
{
	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos))
		return {.result = std::monostate(), .count = 1U};
	return handleThreadFinish(ThreadFinishLabel::create(pos, val, deps));
}

auto GenMCDriver::handleFence(const EventDbgInfo *dbg, Event pos, MemOrdering ord,
			      const EventDeps &deps) -> HandleResult<std::monostate>
{
	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos)) {
		if constexpr (!Config::emitNALabels)
			getExec().getGraph().getState().onFence(
				pos, isAtLeastOrStrongerThan(ord, MemOrdering::Release));
		return {.result = std::monostate{}, .count = 1U};
	}
	return handleFence(FenceLabel::create(pos, ord, deps));
}

auto GenMCDriver::handleLkmmFence(const EventDbgInfo *dbg, Event pos, LkmmFenceKind kind,
				  const EventDeps &deps) -> HandleResult<std::monostate>
{
	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos)) {
		if constexpr (!Config::emitNALabels)
			getExec().getGraph().getState().onFence(
				pos, isAtLeastOrStrongerThan(LkmmFenceLabel::orderingOf(kind),
							     MemOrdering::Release));
		return {.result = std::monostate{}, .count = 1U};
	}
	auto &g = getExec().getGraph();
	auto nesting = getRcuNesting(g, pos);
	auto res = handleFence(LkmmFenceLabel::create(pos, kind, deps));
	if (kind == LkmmFenceKind::RcuUnlock && nesting == 0) {
		reportError(pos, {pos, VerificationError::VE_InvalidRcu,
				  "rcu_read_unlock() without a matching rcu_read_lock()!"});
		return {.result = VerificationError::VE_InvalidRcu};
	}
	/* The grace period would wait for the critical section it is in (the
	 * model makes every such execution inconsistent: none would be seen) */
	if (kind == LkmmFenceKind::SyncRcu && nesting > 0) {
		reportError(pos, {pos, VerificationError::VE_InvalidRcu,
				  "synchronize_rcu() in an RCU read-side critical section!"});
		return {.result = VerificationError::VE_InvalidRcu};
	}
	/* rcu_read_unlock() closes a critical section, which orders events
	 * already in the graph (rcu-rscs): check, as a read that took the
	 * co-maximal write in the section skipped the check */
	if (kind == LkmmFenceKind::RcuUnlock && !getConsChecker().isConsistent(g.getEventLabel(pos)))
		moot();
	return res;
}

auto GenMCDriver::handleSyncSrcu(const EventDbgInfo *dbg, Event pos, SAddr ssp,
				 const EventDeps &deps) -> HandleResult<std::monostate>
{
	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos)) {
		if constexpr (!Config::emitNALabels)
			getExec().getGraph().getState().onFence(pos, true);
		return {.result = std::monostate{}, .count = 1U};
	}
	auto nesting = getRcuNesting(getExec().getGraph(), pos);
	auto res = handleFence(LkmmFenceLabel::create(pos, LkmmFenceKind::SyncSrcu, ssp, deps));
	/* The kernel model's invalid-sleep flag */
	if (nesting > 0) {
		reportError(pos, {pos, VerificationError::VE_InvalidRcu,
				  "synchronize_srcu() in an RCU read-side critical section!"});
		return {.result = VerificationError::VE_InvalidRcu};
	}
	return res;
}

/************************************************************
 ** Memory operation handlers
 ***********************************************************/

#define LOAD_INTERCEPTOR_LOGIC(LabelT, OLD_VAL, ...) /* NOLINT(cppcoreguidelines-macro-usage) */   \
	auto &g = getExec().getGraph();                                                            \
	auto &state = g.getState();                                                                \
	++pos;                                                                                     \
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)                          \
		return {*err};                                                                     \
	if (isExecutionDrivenByGraph(pos)) {                                                       \
		auto *lab = genmc::dyn_cast<ReadLabel>(g.getEventLabel(pos));                      \
		if constexpr (!Config::emitNALabels) {                                             \
			if (OLD_VAL)                                                               \
				g.updateDeferredValue(lab->getAccess(), *OLD_VAL);                 \
			state.onATLoad(lab->getPos(), lab->getAccess(),                            \
				       getConsChecker().getHbView(lab));                           \
		}                                                                                  \
		return getReadRetValue(lab);                                                       \
	}                                                                                          \
	return handleLoad(LabelT::create(pos, __VA_ARGS__), OLD_VAL);

/* Like LOAD_INTERCEPTOR_LOGIC, but tags the label as weak and reports back to
 * the interpreter whether the CAS performs its write (see toCasResult). */
#define CAS_LOAD_INTERCEPTOR_LOGIC(LabelT, OLD_VAL,                                                \
				   ...) /* NOLINT(cppcoreguidelines-macro-usage) */                \
	auto &g = getExec().getGraph();                                                            \
	auto &state = g.getState();                                                                \
	++pos;                                                                                     \
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)                          \
		return {*err};                                                                     \
	if (isExecutionDrivenByGraph(pos)) {                                                       \
		auto *lab = genmc::dyn_cast<ReadLabel>(g.getEventLabel(pos));                      \
		if constexpr (!Config::emitNALabels) {                                             \
			if (OLD_VAL)                                                               \
				g.updateDeferredValue(lab->getAccess(), *OLD_VAL);                 \
			state.onATLoad(lab->getPos(), lab->getAccess(),                            \
				       getConsChecker().getHbView(lab));                           \
		}                                                                                  \
		return toCasResult(getReadRetValue(lab), pos);                                     \
	}                                                                                          \
	auto casLab = LabelT::create(pos, __VA_ARGS__);                                            \
	casLab->setWeak(weak);                                                                     \
	return toCasResult(handleLoad(std::move(casLab), OLD_VAL), pos);

#define STORE_INTERCEPTOR_LOGIC(LabelT, OLD_VAL, ...) /* NOLINT(cppcoreguidelines-macro-usage) */  \
	auto &g = getExec().getGraph();                                                            \
	auto &state = g.getState();                                                                \
	++pos;                                                                                     \
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)                          \
		return {*err};                                                                     \
	if (isExecutionDrivenByGraph(pos)) {                                                       \
		auto *lab = genmc::dyn_cast<WriteLabel>(g.getEventLabel(pos));                     \
		if constexpr (!Config::emitNALabels) {                                             \
			if (OLD_VAL)                                                               \
				g.updateDeferredValue(lab->getAccess(), *OLD_VAL);                 \
			state.onATStore(lab->getPos(), lab->getAccess(),                           \
					getConsChecker().getHbView(lab), lab->isEffectful(),       \
					lab->isAtLeastRelease(), getLockRole(lab));                \
		}                                                                                  \
		return {lab->wasAddedMax(), 1U};                                                   \
	}                                                                                          \
	return handleStore(LabelT::create(pos, __VA_ARGS__), OLD_VAL);

#if EMIT_NA_LABELS
#define GENMC_OLD_VAL_PARAM
#define GENMC_OLD_VAL_PASS std::optional<SVal>()
#else
#define GENMC_OLD_VAL_PARAM std::optional<SVal> oldVal,
#define GENMC_OLD_VAL_PASS oldVal
#endif

#define HANDLE_LOAD_LABEL(name)                                                                    \
	auto GenMCDriver::handle##name(                                                            \
		const EventDbgInfo *dbg, Event pos, GENMC_OLD_VAL_PARAM MemOrdering ord,           \
		SAddr addr, ASize size, EventLabel *rfLab, const std::optional<Annotation> &annot, \
		const EventDeps &deps)                                                             \
		->HandleResult<SVal>                                                               \
	{                                                                                          \
		LOAD_INTERCEPTOR_LOGIC(name##Label, GENMC_OLD_VAL_PASS, ord, addr, size, rfLab,    \
				       annot, deps);                                               \
	}

#define HANDLE_CAS_LOAD_LABEL(name)                                                                \
	auto GenMCDriver::handle##name(                                                            \
		const EventDbgInfo *dbg, Event pos, GENMC_OLD_VAL_PARAM MemOrdering ord,           \
		MemOrdering failOrd, SAddr addr, ASize size, SVal exp, SVal swap, WriteAttr wattr, \
		bool weak, EventLabel *rfLab, const std::optional<Annotation> &annot,              \
		const EventDeps &deps)                                                             \
		->HandleResult<CasReadResult>                                                      \
	{                                                                                          \
		CAS_LOAD_INTERCEPTOR_LOGIC(name##Label, GENMC_OLD_VAL_PASS, ord, failOrd, addr,    \
					   size, exp, swap, wattr, rfLab, annot, deps);            \
	}

#define HANDLE_LOCK_LOAD_LABEL(name)                                                               \
	auto GenMCDriver::handle##name(                                                            \
		const EventDbgInfo *dbg, Event pos, GENMC_OLD_VAL_PARAM SAddr addr, ASize size,    \
		const std::optional<Annotation> &annot, const EventDeps &deps)                     \
		->HandleResult<SVal>                                                               \
	{                                                                                          \
		LOAD_INTERCEPTOR_LOGIC(name##Label, GENMC_OLD_VAL_PASS, addr, size, annot, deps);  \
	}

#define HANDLE_FAI_LOAD_LABEL(name)                                                                \
	auto GenMCDriver::handle##name(                                                            \
		const EventDbgInfo *dbg, Event pos, GENMC_OLD_VAL_PARAM MemOrdering ord,           \
		SAddr addr, ASize size, RMWBinOp op, SVal val, WriteAttr wattr, EventLabel *rfLab, \
		const std::optional<Annotation> &annot, const EventDeps &deps)                     \
		->HandleResult<SVal>                                                               \
	{                                                                                          \
		LOAD_INTERCEPTOR_LOGIC(name##Label, GENMC_OLD_VAL_PASS, ord, addr, size, op, val,  \
				       wattr, rfLab, annot, deps);                                 \
	}

#define HANDLE_STORE_LABEL(name)                                                                   \
	auto GenMCDriver::handle##name(                                                            \
		const EventDbgInfo *dbg, Event pos, GENMC_OLD_VAL_PARAM MemOrdering ord,           \
		SAddr addr, ASize size, SVal val, WriteAttr wattr, const EventDeps &deps)          \
		->HandleResult<bool>                                                               \
	{                                                                                          \
		STORE_INTERCEPTOR_LOGIC(name##Label, GENMC_OLD_VAL_PASS, ord, addr, size, val,     \
					wattr, deps);                                              \
	}

#define HANDLE_LOCK_STORE_LABEL(name)                                                              \
	auto GenMCDriver::handle##name(const EventDbgInfo *dbg, Event pos, SAddr addr, ASize size, \
				       const EventDeps &deps)                                      \
		->HandleResult<bool>                                                               \
	{                                                                                          \
		STORE_INTERCEPTOR_LOGIC(name##Label, std::optional<SVal>(), addr, size, deps);     \
	}

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define HANDLE_CAS_STORE_LABEL(name) HANDLE_STORE_LABEL(name)
#define HANDLE_FAI_STORE_LABEL(name) HANDLE_STORE_LABEL(name)
// NOLINTEND(cppcoreguidelines-macro-usage)

#include "genmc/Execution/EventLabel.def"

#undef GENMC_OLD_VAL_PARAM
#undef GENMC_OLD_VAL_PASS
#undef LOAD_INTERCEPTOR_LOGIC
#undef CAS_LOAD_INTERCEPTOR_LOGIC
#undef STORE_INTERCEPTOR_LOGIC

auto GenMCDriver::handleNALoad(const EventDbgInfo *dbg, Event pos, SAddr loc, ASize size,
			       const EventDeps &deps) -> GenMCDriver::NALoadResult
{
	auto &g = getExec().getGraph();
	[[maybe_unused]] auto &state = g.getState();

	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, true, dbg); err)
		return {*err};
	if (isExecutionDrivenByGraph(pos)) {
		/* TODO: case analysis for view calculation (+updateIdx?) */
#if EMIT_NA_LABELS
		return getReadRetValue(g.getReadLabel(pos));
#else
		state.onNALoad(pos, {loc, size},
			       getConsChecker().getHbView(g.getEventLabel(pos.prev())));
		return {std::monostate(), 0U};
#endif
	}
	return handleNALoad(pos, loc, size, deps);
}

#if EMIT_NA_LABELS
#define GENMC_NA_STORE_VAL_PARAM SVal val,
#else
#define GENMC_NA_STORE_VAL_PARAM
#endif

auto GenMCDriver::handleNAStore(const EventDbgInfo *dbg, Event pos, SAddr loc, ASize size,
				GENMC_NA_STORE_VAL_PARAM const EventDeps &deps)
	-> GenMCDriver::NAStoreResult
{
	auto &g = getExec().getGraph();
	[[maybe_unused]] auto &state = g.getState();

	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, true, dbg); err)
		return {*err};

#if EMIT_NA_LABELS
	const std::optional<SVal> naVal = val;
#else
	const std::optional<SVal> naVal = std::nullopt;
#endif

	if (isExecutionDrivenByGraph(pos)) {
		/* TODO: case analysis for view calculation (+updateIdx?) */
#if EMIT_NA_LABELS
		return {true, 1U};
#else
		state.onNAStore(pos, {loc, size},
				getConsChecker().getHbView(g.getEventLabel(pos.prev())), naVal);
		return {std::monostate(), 0U};
#endif
	}
	return handleNAStore(pos, loc, size, naVal, deps);
}

auto GenMCDriver::handleMalloc(const EventDbgInfo *dbg, Event pos, ASize size, uint64_t alignment,
			       StorageDuration sdur, StorageType styp, AddressSpace spc,
			       const NameInfo *info, const std::string &name, const EventDeps &deps,
			       bool zeroed, bool threadPrivate) -> HandleResult<SVal>
{
	auto &g = getExec().getGraph();

	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, true, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos)) {
		SAddr addr;
		if constexpr (Config::emitNALabels) {
			auto *lab = genmc::dyn_cast<MallocLabel>(g.getEventLabel(pos));
			VERIFY(lab);
			auto oldAddr = lab->getAddr();
			addr = lab->getAddr();
			VERIFY(getConf()->isDepTrackingModel || oldAddr == SAddr() ||
			       oldAddr == lab->getAddr());
		} else {
			addr = g.getState().onAlloc(pos, size, alignment, sdur, styp, spc);
			if (zeroed && addr != SAddr())
				g.getState().onZeroedAlloc(
					pos, {addr, size},
					getConsChecker().getHbView(g.getEventLabel(pos.prev())));
		}
		return {.result = SVal(addr.get()), .count = Config::emitNALabels ? 1U : 0U};
	}

	return handleMalloc(pos, size, alignment, sdur, styp, spc, info, name, deps, zeroed,
			    threadPrivate);
}

auto GenMCDriver::handleRetire(const EventDbgInfo *dbg, Event pos, SAddr loc, const EventDeps &deps)
	-> HandleResult<std::monostate>
{
	ERROR_ON(inRandomMode(), "Hazard pointers are not supported in --mode=random\n");

	auto &g = getExec().getGraph();

	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, true, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos)) {
		if constexpr (!Config::emitNALabels)
			g.getState().onFree(pos, loc, true);
		return {.result = std::monostate(), .count = Config::emitNALabels ? 1U : 0U};
	}

	return handleRetire(pos, loc, deps);
}

auto GenMCDriver::handleFree(const EventDbgInfo *dbg, Event pos, SAddr loc, StorageDuration sdur,
			     const EventDeps &deps) -> HandleResult<std::monostate>
{
	auto &g = getExec().getGraph();

	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, true, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos)) {
		if constexpr (!Config::emitNALabels)
			g.getState().onFree(pos, loc, false);
		return {.result = std::monostate(), .count = Config::emitNALabels ? 1U : 0U};
	}
	return handleFree(pos, loc, sdur, deps);
}

#if EMIT_NA_LABELS
auto GenMCDriver::allocateGlobal(ASize size, uint64_t alignment, const void *initData,
				 bool persistent, bool internal, bool threadLocal) -> SAddr
#else
auto GenMCDriver::allocateGlobal(ASize size, uint64_t alignment, bool persistent, bool internal,
				 bool threadLocal) -> SAddr
#endif
{
	auto &state = getExec().getGraph().getState();
	auto addr = state.addStaticRange(size, alignment, persistent, internal, threadLocal);
#if EMIT_NA_LABELS
	state.addStaticInitData(addr, size, initData);
#endif
	return addr;
}

auto GenMCDriver::handleAssume(const EventDbgInfo *dbg, Event pos, AssumeType type)
	-> HandleResult<std::monostate>
{
	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos))
		return {.result = std::monostate(), .count = 1U};
	return handleBlock(BlockLabel::createAssumeBlock(pos, type));
}

auto GenMCDriver::handleHelpingCas(const EventDbgInfo *dbg, Event pos, MemOrdering ord, SAddr loc,
				   ASize size, SVal cmpVal, SVal newVal, const EventDeps &deps)
	-> HandleResult<std::monostate>
{
	ERROR_ON(inRandomMode(),
		 "Helped/Helping CAS annotations are not supported in --mode=random\n");
	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos))
		return {.result = std::monostate(), .count = 1U};
	return handleHelpingCas(HelpingCasLabel::create(pos, ord, loc, size, cmpVal, newVal, deps));
}

auto GenMCDriver::handleOptional(const EventDbgInfo *dbg, Event pos) -> HandleResult<bool>
{
	auto &g = getExec().getGraph();

	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos))
		return {.result =
				genmc::dyn_cast<OptionalLabel>(g.getEventLabel(pos))->isExpanded(),
			.count = 1U};
	return handleOptional(OptionalLabel::create(pos));
}

auto GenMCDriver::handleSpinStart(const EventDbgInfo *dbg, Event pos, LoopKey loop)
	-> HandleResult<std::monostate>
{
	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos)) {
		if constexpr (!Config::emitNALabels)
			getExec().getGraph().getState().onSpinStart(pos, loop);
		return {.result = std::monostate(), .count = 1U};
	}
	return handleSpinStart(SpinStartLabel::create(pos, loop));
}

auto GenMCDriver::handleAwaitStart(const EventDbgInfo *dbg, Event pos)
	-> HandleResult<std::monostate>
{
	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos)) {
		if constexpr (!Config::emitNALabels)
			getExec().getGraph().getState().onAwaitStart(pos);
		return {.result = std::monostate(), .count = 1U};
	}
	return handleAwaitStart(AwaitStartLabel::create(pos));
}

auto GenMCDriver::handleFaiZNESpinEnd(const EventDbgInfo *dbg, Event pos)
	-> HandleResult<std::monostate>
{
	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos))
		return {.result = std::monostate(), .count = 1U};
	return handleFaiZNESpinEnd(FaiZNESpinEndLabel::create(pos));
}

auto GenMCDriver::handleLockZNESpinEnd(const EventDbgInfo *dbg, Event pos)
	-> HandleResult<std::monostate>
{
	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos))
		return {.result = std::monostate(), .count = 1U};
	return handleLockZNESpinEnd(LockZNESpinEndLabel::create(pos));
}

auto GenMCDriver::handleLoopBegin(const EventDbgInfo *dbg, Event pos, LoopKey loop)
	-> HandleResult<std::monostate>
{
	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos)) {
		if constexpr (!Config::emitNALabels)
			getExec().getGraph().getState().onLoopBegin(pos, loop);
		return {.result = std::monostate(), .count = 1U};
	}
	return handleLoopBegin(LoopBeginLabel::create(pos, loop));
}
auto GenMCDriver::handleHpProtect(const EventDbgInfo *dbg, Event pos, SAddr hpAddr, SAddr protAddr)
	-> HandleResult<std::monostate>
{
	ERROR_ON(inRandomMode(), "Hazard pointers are not supported in --mode=random\n");

	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos)) {
		if constexpr (!Config::emitNALabels)
			getExec().getGraph().getState().onProtect(hpAddr, protAddr);
		return {.result = std::monostate(), .count = 1U};
	}
	getExec().getGraph().getState().onProtect(hpAddr, protAddr);
	return handleDummy(HpProtectLabel::create(pos, hpAddr, protAddr));
}
auto GenMCDriver::handleMethodBegin(const EventDbgInfo *dbg, Event pos,
				    const std::string &methodName, int32_t argVal)
	-> HandleResult<std::monostate>
{
	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos))
		return {.result = std::monostate(), .count = 1U};
	return handleDummy(MethodBeginLabel::create(pos, methodName, argVal));
}

auto GenMCDriver::handleMethodEnd(const EventDbgInfo *dbg, Event pos, const std::string &methodName,
				  int32_t retVal) -> HandleResult<std::monostate>
{
	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos))
		return {.result = std::monostate(), .count = 1U};
	return handleDummy(MethodEndLabel::create(pos, methodName, retVal));
}

auto GenMCDriver::handleOutput(const EventDbgInfo *dbg, Event pos, std::string msg)
	-> HandleResult<std::monostate>
{
	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos))
		return {.result = std::monostate(), .count = 1U};
	return handleDummy(OutputLabel::create(pos, std::move(msg)));
}

auto GenMCDriver::handleError(const EventDbgInfo *dbg, Event pos, std::string msg)
	-> HandleResult<std::monostate>
{
	++pos;
	if (auto err = updateErrorInfoAndMaybeExit(pos, false, dbg); err)
		return {.result = *err};
	if (isExecutionDrivenByGraph(pos))
		return {.result = std::monostate(), .count = 1U};
	auto result = handleDummy(ErrorLabel::create(pos, msg));
	VERIFY(std::holds_alternative<std::monostate>(result.result));
	reportError(pos, {pos, VerificationError::VE_Safety, std::move(msg)});
	return {.result = VerificationError::VE_Safety, .count = 1U};
}
auto GenMCDriver::getDbgInfo(Event pos) -> const GenMCDriver::EventDbgInfo *
{
	if (!errorReplay_->dbgInfo().contains(pos))
		return nullptr;
	return &errorReplay_->dbgInfo().at(pos);
}
