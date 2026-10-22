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
#include "genmc/Execution/Consistency/BoundDecider.hpp"
#include "genmc/Execution/Consistency/ConsistencyChecker.hpp"
#include "genmc/Execution/Consistency/SymmetryChecker.hpp"
#include "genmc/Execution/DepExecutionGraph.hpp"
#include "genmc/Execution/DepInfo.hpp"
#include "genmc/Execution/EventAttr.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionState.hpp"
#include "genmc/Execution/GraphUtils.hpp"
#include "genmc/Execution/LkmmModel.hpp"
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
#include "genmc/Verification/DriverHandlerDispatcher.hpp"
#include "genmc/Verification/GraphPrinting.hpp"
#include "genmc/Verification/Invariants.hpp"
#include "genmc/Verification/Relinche/LinearizabilityChecker.hpp"
#include "genmc/Verification/Relinche/Specification.hpp"
#include "genmc/Verification/Revisit.hpp"
#include "genmc/Verification/Scheduler.hpp"
#include "genmc/Verification/VerificationError.hpp"
#include "genmc/Verification/VerificationResult.hpp"
#include "genmc/Verification/WorkList.hpp"
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

GenMCDriver::GenMCDriver(std::shared_ptr<const Config> conf, WorkSharing *pool /* = nullptr */,
			 Mode mode /* = VerificationMode{} */)
	: mode(mode), pool(pool), userConf(std::move(conf))
{
	/* Set up the execution context */
	consChecker = ConsistencyChecker::create(getConf());
	auto cfg = ExecutionGraph::Config{.execState = &execState_,
					  .memModel = &*consChecker,
					  .emitNALabels = Config::emitNALabels};
	auto execGraph = userConf->isDepTrackingModel ? std::make_unique<DepExecutionGraph>(cfg)
						      : std::make_unique<ExecutionGraph>(cfg);
	execStack.emplace_back(std::move(execGraph), LocalQueueT(), ChoiceMap());

	symmChecker = SymmetryChecker::create();
	auto hasBounder = userConf->bound.has_value();
	GENMC_DEBUG(hasBounder |= userConf->boundsHistogram;);
	if (hasBounder)
		bounder = BoundDecider::create(getConf()->boundType);

	scheduler_ = (inEstimationMode() || inRandomMode())
			     ? std::make_unique<WFRScheduler>(getConf())
			     : Scheduler::create(getConf());

	/* Set up a random-number generator (for the scheduler) */
	if (userConf->printRandomScheduleSeed) {
		PRINT(VerbosityLevel::Error, "Seed: {}\n", *userConf->randomScheduleSeed);
	}
	estRng.seed(*userConf->randomScheduleSeed);

	if (userConf->collectLinSpec)
		result.specification = std::make_unique<Specification>(userConf->maxExtSize
#ifdef ENABLE_GENMC_DEBUG
								       ,
								       userConf->relincheDebug
#endif
		);
	if (userConf->checkLinSpec)
		relinche =
			LinearizabilityChecker::create(&getConsChecker(), *getConf()->checkLinSpec);
}

GenMCDriver::~GenMCDriver() = default;


static void repairRead(ExecutionGraph &g, ReadLabel *lab)
{
	auto *maxLab = g.co_max(lab->getAddr());
	lab->setRf(maxLab);
}

void GenMCDriver::repairDanglingReads(ExecutionGraph &g)
{
	for (auto i = 0; i < g.getNumThreads(); i++) {
		auto *rLab = genmc::dyn_cast<ReadLabel>(g.getLastThreadLabel(i));
		if (rLab && !rLab->getRf()) {
			repairRead(g, rLab);
			updateLabelViews(rLab);
		}
	}
}


void GenMCDriver::pushExecution(Execution &&e)
{
	execStack.push_back(std::move(e));
	getConsChecker().recomputeCacheCounters(getExec().getGraph());
}

auto GenMCDriver::invariantContext(std::string_view where) const -> invariants::Context
{
	return {.where = where,
		.dbgInfo = &dbgInfo_,
		.holesAllowed = getConf()->isDepTrackingModel};
}

void GenMCDriver::checkGraphInvariants(const ExecutionGraph &g, std::string_view where,
				       const invariants::RestrictionPlan *plan) const
{
	if (!checksInvariants())
		return;
	invariants::checkGraph(g, invariantContext(where), plan);
}

void GenMCDriver::checkAllocationInvariants(std::string_view where) const
{
	if (!checksInvariants())
		return;
	const auto &g = getExec().getGraph();
	invariants::checkAllocations(g, g.getState(), invariantContext(where));
}

void GenMCDriver::restrictExecution(Stamp stamp, std::string_view where)
{
	if (!checksInvariants()) {
		getExec().restrict(stamp);
		return;
	}
	auto plan = invariants::planCutToStamp(getExec().getGraph(), stamp);
	getExec().restrict(stamp);
	checkGraphInvariants(getExec().getGraph(), where, &plan);
}

/* The role of a write in a mutex, for spinloop detection */
static auto getLockRole(const WriteLabel *wLab) -> ExecutionState::LockRole
{
	if (genmc::isa<LockCasWriteLabel>(wLab))
		return ExecutionState::LockRole::Acquire;
	if (genmc::isa<UnlockWriteLabel>(wLab))
		return ExecutionState::LockRole::Release;
	return ExecutionState::LockRole::None;
}

auto GenMCDriver::popExecution() -> bool
{
	if (execStack.empty())
		return false;
	execStack.pop_back();
	if (execStack.empty())
		return false;
	getConsChecker().recomputeCacheCounters(getExec().getGraph());
	return true;
}

void GenMCDriver::initFromState(std::unique_ptr<Execution> exec)
{
	/* A task is only taken once the previous one is done, and a done task
	 * has reported its errors (or the pool halted and no task is taken):
	 * a replay still pending here is stale (see -check-invariants, I2) */
	if (checksInvariants() && (getScheduler().inErrorReplay() || bufferedError_)) {
		auto target = getScheduler().getErrorReplayEvent();
		invariants::fail("I2 (error replay)",
				 std::format("a new task is taken with an error replay pending "
					     "(target {}, buffered error: {})",
					     target ? std::format("{}", *target) : "none",
					     bufferedError_.has_value()),
				 execStack.empty() ? static_cast<const ExecutionGraph *>(nullptr)
						   : &getExec().getGraph(),
				 invariantContext("taking a task from the pool"));
	}
	checkGraphInvariants(*exec->graph, "taking a task from the pool");

	/* A replay that collects an error's metadata belongs to the executions
	 * being replaced: drop it, or the next schedule would replay to an event
	 * of the old graph. (It is cut short only when the pool halts, and then
	 * the error is reported by the worker that halted it.) */
	getScheduler().setErrorReplayEvent(std::nullopt);
	bufferedError_ = std::nullopt;
	dbgInfo_.clear();
	pendingResume_ = nullptr;

	execStack.clear();
	execStack.emplace_back(std::move(exec->graph), LocalQueueT(), std::move(exec->choices));

	getExec().getGraph().setMemModel(&getConsChecker());
	getExec().getGraph().setState(&getExecState());

	getConsChecker().recomputeCacheCounters(getExec().getGraph());
}

auto GenMCDriver::extractState() -> std::unique_ptr<GenMCDriver::Execution>
{
	auto og = getExec().getGraph().clone();
	checkGraphInvariants(*og, "copying the graph for another worker");
	return std::make_unique<Execution>(GenMCDriver::Execution(
		std::move(og), LocalQueueT(), ChoiceMap(getExec().getChoiceMap())));
}

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
	addedBeforeKept_ = false;
	keptCheckPending_ = false;
	droppedBeforeKept_ = false;
	g.setKeptEventsUnreachable(false);
	runStartStamp_ = g.getMaxStamp();
	runStartWork_ = getExec().getWorkqueue().size();
	state.clear();
	if constexpr (Config::emitNALabels)
		reconstructState(g, state);
	if constexpr (Config::emitNALabels)
		checkAllocationInvariants("reconstructing the state");
	/* An error replay replays to its target, which the graph holds (I2) */
	if (checksInvariants() && getScheduler().inErrorReplay()) {
		auto target = *getScheduler().getErrorReplayEvent();
		if (!bufferedError_ || !g.containsPosNonEmpty(target))
			invariants::fail("I2 (error replay)",
					 std::format("an execution starts in error replay but its "
						     "target {} is {} (buffered error: {})",
						     target,
						     g.containsPosNonEmpty(target)
							     ? "in the graph"
							     : "not in the graph",
						     bufferedError_.has_value()),
					 &g, invariantContext("starting an execution"));
	}
	getScheduler().resetExplorationOptions(g);
	/* A resumed execution is driven through the holes before its kept
	 * events as a replayed one is (I4) */
	if (pendingResume_ && checksInvariants())
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
	if (getScheduler().inErrorReplay() && bufferedError_) {
		WARN_ONCE("incomplete-error-replay",
			  "The error report may lack source information (incomplete replay).\n");
		auto details = *bufferedError_;
		reportError(*getScheduler().getErrorReplayEvent(), details);
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
			printGraph(g, dbgInfo_);
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
		printGraph(g, dbgInfo_);

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

void GenMCDriver::maybeCutGraph(const EventLabel *lab)
{
	auto &g = getExec().getGraph();

	/* Restamping keeps the maximum stamp equal to the live-label count.
	 * 3 = ThreadStart + terminator + potential create/join */
	if (g.getMaxStamp().get() <
	    (static_cast<unsigned>(g.getNumThreads()) * 3) + *getConf()->maxGraphSize)
		return;

	/* Don't cut between create-start or RMW pairs */
	const auto *wLab = genmc::dyn_cast<WriteLabel>(lab);
	if (genmc::isa<ThreadCreateLabel>(lab) ||
	    genmc::isa<ThreadCreateLabel>(&*g.rlabels().begin()) || (wLab && wLab->isRMW()))
		return;

	auto es = pickPoMinimalEvents(g);
	if (es.empty())
		return;
	g.cutToView(calcSCPrefixClosure(g, es));
	checkGraphInvariants(g, "pruning the graph");
}

void GenMCDriver::restartExecution()
{
	pendingResume_ = nullptr;
	auto cfg = ExecutionGraph::Config{.execState = &execState_,
					  .memModel = &*consChecker,
					  .emitNALabels = Config::emitNALabels};
	auto execGraph = getConf()->isDepTrackingModel ? std::make_unique<DepExecutionGraph>(cfg)
						       : std::make_unique<ExecutionGraph>(cfg);
	execStack.clear();
	execStack.emplace_back(std::move(execGraph), LocalQueueT(), ChoiceMap());
	getConsChecker().recomputeCacheCounters(getExec().getGraph());
}

auto GenMCDriver::done() -> bool
{
	/* Repeat a run whose thread's kept events were moved: from its start
	 * (the events added since are cut), so that every structure built during
	 * the run sees the new positions */
	if (rerunAfterRelocation_ && !isHalting()) {
		rerunAfterRelocation_ = false;
		++result.relocationReruns;
		static constexpr auto kMaxRelocations = 64U;
		ERROR_ON(++relocationsInRun_ > kMaxRelocations,
			 "Internal error: a thread's kept events do not settle\nPlease submit a "
			 "bug report to {}\n",
			 PACKAGE_BUGREPORT);
		getExec().getWorkqueue().truncate(runStartWork_);
		restrictExecution(runStartStamp_,
				  "restricting the graph to rerun after a relocation");
		getConsChecker().recomputeCacheCounters(getExec().getGraph());
		repairDanglingReads(getExec().getGraph());
		return false;
	}
	/* Repeat a run that was inconsistent after an event went before kept
	 * ones, checking after each such event (the work it added is cut) */
	if (rerunEagerly_ && !isHalting()) {
		rerunEagerly_ = false;
		eagerKeptChecks_ = true;
		++result.keptCheckReruns;
		getExec().getWorkqueue().truncate(runStartWork_);
		restrictExecution(runStartStamp_,
				  "restricting the graph to rerun with eager checks");
		getConsChecker().recomputeCacheCounters(getExec().getGraph());
		repairDanglingReads(getExec().getGraph());
		return false;
	}
	relocationsInRun_ = 0;
	eagerKeptChecks_ = false;

	auto validExecution = false;
	while (!isHalting() && !validExecution) {
		auto item = getExec().getWorkqueue().getNext();
		if (!item) {
			/* Random mode never revisits: resample while the
			 * budget lasts (error replays are still queued) */
			if (inRandomMode()) {
				if (shouldStopRandom())
					return true;
				restartExecution();
				return false;
			}
			if (popExecution())
				continue;
			return true;
		}
		validExecution = restrictAndRevisit(item) && isRevisitValid(*item) &&
				 visitRevisitedGraph(*item);
	}
	return isHalting();
}

/* Without the maximality check (-exhaustive), TruSt may produce a graph it
 * produced before: w1 revisits r; w2, re-added, revisits r back to the graph
 * where r read from w2. The exploration from a graph depends on the graph
 * alone (every read is revisitable, no flag set on the way matters), so a
 * repeated graph adds nothing, and skipping it is what makes the exploration
 * terminate. Error replays re-execute a graph on purpose. */
auto GenMCDriver::visitRevisitedGraph(const Revisit &item) -> bool
{
	if (!getConf()->exhaustive || genmc::isa<ReplayForwardRevisit>(&item) ||
	    genmc::isa<RerunForwardRevisit>(&item))
		return true;
	if (visitedGraphs_.insert(canonicalGraphHash(getExec().getGraph())).second) {
		++result.graphsVisited;
		return true;
	}
	++result.revisitsRepeated;
	/* A backward revisit pushed an execution of its own: drop the work its
	 * RMW completion may have queued, so that the execution is popped */
	if (genmc::isa<BackwardRevisit>(&item))
		getExec().getWorkqueue().truncate(0);
	return false;
}

auto GenMCDriver::isHalting() const -> bool
{
	auto *pool = getThreadPool();
	return shouldHalt || (pool && pool->shouldHalt());
}

void GenMCDriver::halt(VerificationError status)
{
	shouldHalt = true;
	result.status = status;
	if (getThreadPool())
		getThreadPool()->halt();
}

/************************************************************
 ** Scheduling methods
 ***********************************************************/

void GenMCDriver::blockThreadTryMoot(std::unique_ptr<BlockLabel> bLab)
{
	auto &g = getExec().getGraph();
	auto pos = bLab->getPos();
	/* (A block before kept events goes after them: no previous access to
	 * judge by, see blockThread()) */
	auto beforeKept = g.hasEventsAfter(pos.thread, pos.index);
	blockThread(g, std::move(bLab));
	if (beforeKept)
		return;
	auto *lab = g.getLastThreadLabel(pos.thread);
	mootExecutionIfFullyBlocked(lab);
}

auto GenMCDriver::checkPendingKept() -> bool
{
	if (!keptCheckPending_)
		return true;
	keptCheckPending_ = false;
	if (getConsChecker().isConsistent(getExec().getGraph()))
		return true;
	rerunEagerly_ = true;
	moot();
	return false;
}

auto GenMCDriver::scheduleNext(std::span<Action> runnable) -> ScheduleResult
{
	auto &g = getExec().getGraph();
	if (addedBeforeKept_ && !isMoot()) {
		addedBeforeKept_ = false;
		if (!eagerKeptChecks_)
			keptCheckPending_ = true;
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

	if (auto next = getScheduler().schedule(g, runnable); next)
		return *next;
	return g.isBlocked() ? ScheduleResult(Blocked{}) : ScheduleResult(Finished{});
}

auto GenMCDriver::runFromCache() -> bool
{
	if (!getConf()->instructionCaching || !inVerificationMode())
		return false;

	do {
		auto toAdd = getScheduler().scheduleFromCache(getExec().getGraph());
		if (!toAdd)
			return true;
		if (*toAdd == nullptr)
			return false;

		addLabelsToGraph(**toAdd);
	} while (!isMoot() && !isHalting());
	return true;
}

auto GenMCDriver::isExecutionValid(const EventLabel *lab) -> bool
{
	const auto *rLab = genmc::dyn_cast<ReadLabel>(lab);
	return (!getConf()->symmetryReduction || getSymmChecker().isSymmetryOK(lab)) &&
	       ((rLab && rLab->getRf() == getExec().getGraph().co_max(rLab->getAddr()) &&
		 getConsChecker().isCoMaxReadConsistent(rLab)) ||
		getConsChecker().isConsistent(lab)) &&
	       !partialExecutionExceedsBound();
}

auto GenMCDriver::isRevisitValid(const Revisit &revisit) -> bool
{
	auto &g = getExec().getGraph();
	auto pos = revisit.getPos();
	auto *mLab = genmc::dyn_cast<MemAccessLabel>(g.getEventLabel(pos));

	/* For replay/optional revisits, do nothing.
	 * (For replays, it is crucial: the graph might not be well-formed; e.g., invalid access) */
	if (genmc::isa<ReplayForwardRevisit>(&revisit) || !mLab)
		return true;

	if (!isExecutionValid(mLab))
		return false;

	/* If an extra event is added, re-check consistency */
	auto *rLab = genmc::dyn_cast<ReadLabel>(mLab);
	auto *nLab = g.po_imm_succ(mLab);
	return !rLab || !rLab->isRMW() ||
	       (isExecutionValid(nLab) // && !checkForRaces(nLab) // FIXME
	       );
}

auto GenMCDriver::isExecutionDrivenByGraph(Event curr) -> bool
{
	/* Compare positions and not sizes: pruning skews the event count */
	const auto &g = getExec().getGraph();
	const auto *lab = g.getEventLabelIfPresent(curr);
	auto keyed = currKey && curr.thread == currKeyBase.thread && curr.index > currKeyBase.index;
	if (keyed && relocatesKeptEvents())
		return alignKeptEvents(curr, lab);
	if (lab == nullptr || genmc::isa<EmptyLabel>(lab))
		return false;
	if (keyed)
		checkReplayKey(lab, curr.index - currKeyBase.index);
	return true;
}

auto GenMCDriver::alignKeptEvents(Event curr, const EventLabel *lab) -> bool
{
	auto &g = getExec().getGraph();
	auto sub = curr.index - currKeyBase.index;
	auto isOwn = [&](const EventLabel *oLab) {
		return oLab->getKey() && *oLab->getKey() == *currKey && oLab->getKeySub() == sub;
	};
	/* No such run: it is dropped, and the event being aligned is not added
	 * (handleLoad(), handleStore() and handleThreadFinish() reset their
	 * instruction; the other kinds go into the hole, harmlessly). Before,
	 * the run went on with the event: a second event of the instance,
	 * placed in coherence against the kept ones and so contradicting it,
	 * and the revisits and warnings computed from that graph */
	auto dropRun = [&] {
		++result.relocationsLost;
		droppedBeforeKept_ = true;
		moot();
	};
	auto later = [&g, curr] {
		auto size = g.getThreadSize(curr.thread);
		return std::views::iota(std::min(curr.index + 1, size), size) |
		       std::views::transform([&g, curr](int i) {
			       return g.getEventLabel(Event(curr.thread, i));
		       }) |
		       std::views::filter([](auto *oLab) { return !genmc::isa<EmptyLabel>(oLab); });
	};

	if (lab != nullptr && !genmc::isa<EmptyLabel>(lab)) {
		if (isOwn(lab))
			return true;
		if (!lab->getKey())
			checkReplayKey(lab, sub); /* a missing key is an error still */

		/* A new event before kept ones. If the kept events include this
		 * instance's, the ones in between would be skipped: no such run */
		auto skipsKept = std::ranges::any_of(later(), isOwn);
		if (skipsKept) {
			relocateExecution(curr.thread, curr.index, 1);
			dropRun();
			return false;
		}
		/* Open a gap (doubling on repeats; the surplus is closed later) */
		relocateAndRerun(curr.thread, curr.index, 1 << std::min(relocationsInRun_, 10U));
		return false;
	}

	/* A hole: fill it, unless this instance's event is kept further on:
	 * after holes only (the old path was longer), move it here; after other
	 * kept events, those would be skipped: no such run, as above */
	auto rest = later();
	auto first = std::ranges::begin(rest);
	if (first == std::ranges::end(rest))
		return false;
	if (!isOwn(*first)) {
		if (std::ranges::any_of(rest, isOwn)) {
			dropRun();
			return false;
		}
		addedBeforeKept_ = true;
		return false;
	}
	relocateAndRerun(curr.thread, (*first)->getIndex(), curr.index - (*first)->getIndex());
	return true;
}

void GenMCDriver::relocateExecution(int thread, int from, int delta)
{
	getExec().getGraph().relocateThread(thread, from, delta);
	getExec().getWorkqueue().relocateThread(thread, from, delta);
	getExec().getChoiceMap().relocateThread(thread, from, delta);
	checkGraphInvariants(getExec().getGraph(), "relocating a thread's events");
}

void GenMCDriver::relocateAndRerun(int thread, int from, int delta)
{
	relocateExecution(thread, from, delta);
	rerunAfterRelocation_ = true;
	moot();
}

void GenMCDriver::assignEventKey(EventLabel *lab) const
{
	if (currKey && lab->getThread() == currKeyBase.thread &&
	    lab->getIndex() > currKeyBase.index)
		lab->setKey(currKey, lab->getIndex() - currKeyBase.index);
	else
		lab->setKey(nullptr, 0);
}

void GenMCDriver::checkReplayKey(const EventLabel *lab, std::uint32_t sub) const
{
	if (lab->getKey() && *lab->getKey() == *currKey && lab->getKeySub() == sub)
		return;
	ERROR("Internal error: replaying {} reuses the label of another instruction instance "
	      "({} #{}, now {} #{}): {}\nPlease submit a bug report to {}\n",
	      lab->getPos(), lab->getKey() ? lab->getKey()->toString() : "none",
	      lab->getKeySub(), currKey->toString(), sub, *lab, PACKAGE_BUGREPORT);
}

auto GenMCDriver::executionExceedsBound(BoundCalculationStrategy strategy) const -> bool
{
	if (!getConf()->bound.has_value() || inEstimationMode())
		return false;

	return bounder->doesExecutionExceedBound(getExec().getGraph(), *getConf()->bound, strategy);
}

auto GenMCDriver::fullExecutionExceedsBound() const -> bool
{
	return executionExceedsBound(BoundCalculationStrategy::NonSlacked);
}

auto GenMCDriver::partialExecutionExceedsBound() const -> bool
{
	return executionExceedsBound(BoundCalculationStrategy::Slacked);
}

auto GenMCDriver::addLabelToGraph(std::unique_ptr<EventLabel> lab) -> EventLabel *
{
	auto &g = getExec().getGraph();

	/* Cache the event before updating views (inits are added w/ tcreate) */
	if (getConf()->instructionCaching && inVerificationMode())
		getScheduler().cacheEventLabel(g, &*lab);

	/* Prune the graph when it exceeds the window */
	if (inRandomMode() && getConf()->maxGraphSize.has_value())
		maybeCutGraph(&*lab);

	/* (A label may come with its key: a cached one, an RMW's write) */
	if (!lab->getKey())
		assignEventKey(&*lab);

	/* Add and update views */
	auto *addedLab = g.add(std::move(lab));
	updateLabelViews(addedLab);

	if (std::cmp_greater_equal(addedLab->getIndex(), getConf()->warnOnGraphSize)) {
		LOG_ONCE("large-graph", VerbosityLevel::Tip,
			 "The execution graph seems quite large. Consider bounding all loops or "
			 "using -unroll\n");
	}
	return addedLab;
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

void GenMCDriver::updateLabelViews(EventLabel *lab)
{
	getConsChecker().updateMMViews(lab);
	if (!getConf()->symmetryReduction)
		return;

	getSymmChecker().updatePrefixWithSymmetries(lab);
}

void GenMCDriver::configureProbe(MemLabel *rLab, Event pos, SAddr addr, ASize size,
				 const EventDeps &deps)
{
	rLab->reset();

	rLab->setPos(pos);
	assignEventKey(rLab);
	rLab->setAddr(addr);
	rLab->setSize(size);
	rLab->setDeps(deps);

	auto &g = getExec().getGraph();
	/* Dependency-tracking threads may contain holes past POS; pruning (never
	 * under dependency tracking) may have removed the predecessor's prefix */
	auto *predLab = getConf()->isDepTrackingModel ? g.getEventLabel(pos.prev())
						      : g.getLastThreadLabel(pos.thread);
	VERIFY(predLab->getIndex() == pos.index - 1 || g.wasPruned());
	VERIFY(!g.containsPosNonEmpty(pos));
	VERIFY(!genmc::isa<EmptyLabel>(predLab));
	for (const auto &v : predLab->views()) {
		auto newV = View(v);
		newV.updateIdx(pos);
		rLab->addView(std::move(newV));
	}
	rLab->setPrefixView(getPrefixView(predLab).clone());
}

/* If LAB accesses another thread's copy of a thread-local (see SAddr),
 * returns that thread, and 0 otherwise. The copies of a thread end with it,
 * as if freed by its end; those of the main thread (0) last as long as the
 * program */
static auto getThreadLocalOwner(const EventLabel *lab) -> int
{
	const auto *mLab = genmc::dyn_cast<MemAccessLabel>(lab);
	if (!mLab || !mLab->getAddr().isStatic())
		return 0;
	auto owner = mLab->getAddr().getThread();
	return owner == lab->getThread() || owner >= lab->getParent()->getNumThreads() ? 0 : owner;
}

auto GenMCDriver::checkThreadLocalEnd(const EventLabel *lab, const ThreadFinishLabel *eLab)
	-> std::optional<VerificationError>
{
	if (getConsChecker().getHbView(eLab).contains(lab->getPos()))
		return {};
	reportError(lab->getPos(),
		    {lab->getPos(), VerificationError::VE_AccessFreed,
		     std::format("The access is to a thread-local of thread {}, and does not "
				 "happen before the thread ends\n",
				 eLab->getThread()),
		     eLab});
	return {VerificationError::VE_AccessFreed};
}

auto GenMCDriver::checkEndedThreadLocals(const ThreadFinishLabel *eLab)
	-> std::optional<VerificationError>
{
	/* (The main thread's copies last as long as the program; an owner of 0
	 * is no owner, see getThreadLocalOwner()) */
	const auto &g = getExec().getGraph();
	if (eLab->getThread() == 0 || getConf()->disableRaceDetection || inEstimationMode() ||
	    !g.getState().hasThreadLocals())
		return {};

	for (const auto &lab : g.labels()) {
		if (getThreadLocalOwner(&lab) != eLab->getThread())
			continue;
		if (auto err = checkThreadLocalEnd(&lab, eLab); err)
			return err;
	}
	return {};
}

auto GenMCDriver::checkForRaces(const EventLabel *lab) -> std::optional<VerificationError>
{
	if (inEstimationMode())
		return {};

	/* An access to the thread-local of a thread that has ended (see also
	 * checkEndedThreadLocals()) */
	const auto &g = getExec().getGraph();
	if (auto owner = getThreadLocalOwner(lab); owner != 0) {
		const auto *eLab = genmc::dyn_cast<ThreadFinishLabel>(g.getLastThreadLabel(owner));
		if (auto err = eLab ? checkThreadLocalEnd(lab, eLab) : std::nullopt; err)
			return err;
	}

	/* -exhaustive under IMM: what the views leave unordered is checked
	 * again with the views computed anew (see checkWithViewsAnew()) */
	const auto checksAnew = getConf()->exhaustive && getConf()->model == ModelType::IMM;
	auto inGraph = [&g](const EventLabel *oLab) {
		return oLab ? g.getEventLabel(oLab->getPos()) : nullptr;
	};

	/* Check for hard errors */
	if (!getConf()->disableRaceDetection) {
		const EventLabel *racyLab = nullptr;
		auto err = getConsChecker().checkErrors(lab, racyLab);
		auto checkAnew = [&](const ExecutionGraph &cg) {
			err = getConsChecker().checkErrors(cg.getEventLabel(lab->getPos()),
							   racyLab);
			racyLab = inGraph(racyLab);
		};
		if (err && checksAnew)
			checkWithViewsAnew(checkAnew);
		if (err) {
			reportError(lab->getPos(), {lab->getPos(), *err, "", racyLab});
			return err;
		}
	}

	/* Unordered writes are an error under SR and IPR once the graph has
	 * symmetric threads, or annotated reads of their location (see
	 * reportWarningOnce()), which it may only have after the first ones,
	 * depending on the schedule: while SR or IPR is on, they are checked at
	 * every write, and without race detection too (#130) */
	auto checksWWRaces = getConf()->symmetryReduction || getConf()->ipr;
	if (getConf()->disableRaceDetection && !checksWWRaces)
		return {};

	/* Check whether there are any unreported warnings... */
	const auto &known = getResult().warnings;
	auto recheck = checksWWRaces && known.contains(VerificationError::VE_WWRace);
	auto unordered = known;
	if (recheck)
		unordered.erase(VerificationError::VE_WWRace);
	std::vector<const EventLabel *> races;
	auto newWarnings = getConsChecker().checkWarnings(lab, recheck ? unordered : known, races);
	auto checkAnew = [&](const ExecutionGraph &cg) {
		races.clear();
		newWarnings = getConsChecker().checkWarnings(cg.getEventLabel(lab->getPos()),
							     recheck ? unordered : known, races);
		std::ranges::transform(races, races.begin(), inGraph);
	};
	if (!newWarnings.empty() && checksAnew)
		checkWithViewsAnew(checkAnew);

	/* ... and report them */
	auto i = 0;
	for (auto &wcode : newWarnings) {
		const auto *racyLab = races[i++];
		if (getConf()->disableRaceDetection && wcode != VerificationError::VE_WWRace)
			continue;
		if (reportWarningOnce(lab->getPos(), wcode, racyLab))
			return {wcode};
	}
	return {};
}

auto GenMCDriver::getReadRetValue(const ReadLabel *rLab) -> HandleResult<SVal>
{
	/* Bottom is an acceptable re-option only @ replay */
	auto &scheduler = getScheduler();
	if (!rLab->getRf()) {
		VERIFY(scheduler.inErrorReplay());
		return {.result = Invalid{}, .count = 1U};
	}

	using Result = HandleResult<SVal>;
	using Evaluator = SExprEvaluator<ModuleVarID>;
	auto res = rLab->getAccessValue(rLab->getAccess());
	auto &g = getExec().getGraph();

	/* Return nullopt for reads that require an interpreter reset */
	if (getConf()->ipr && rLab->getAnnot() &&
	    !Evaluator().evaluate(&*rLab->getAnnot()->expr, res)) {
		auto bLab = BlockLabel::createAssumeBlock(rLab->getPos().next(),
							  rLab->getAnnot()->type);
		inheritDbgInfo(rLab->getPos(), &*bLab);
		blockThread(g, std::move(bLab));
		return scheduler.inErrorReplay() ? Result(Invalid(), 1U) : Result(Reset(), 0U);
	}
	if (genmc::isa<BWaitReadLabel>(rLab) &&
	    !readsBarrierUnblockingValue(genmc::cast<BWaitReadLabel>(rLab))) {
		auto bLab =
			BlockLabel::createAssumeBlock(rLab->getPos().next(), AssumeType::Barrier);
		inheritDbgInfo(rLab->getPos(), &*bLab);
		blockThread(g, std::move(bLab));
		return scheduler.inErrorReplay() ? Result(Invalid(), 1U) : Result(Reset(), 0U);
	}
	return {.result = res, .count = 1U};
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

/* The rfs getRfsApproximation() drops are the writes whose successful RMW
 * would violate atomicity (a settled RMW reads them already). A weak CAS can
 * still read such a write and fail spuriously: that execution has no write to
 * violate anything, and no other path explores it */
void GenMCDriver::pushWeakCasFailuresOnDroppedRfs(ReadLabel *lab,
						  const std::vector<EventLabel *> &kept)
{
	const auto *cLab = genmc::dyn_cast<CasReadLabel>(lab);
	if (!cLab || !cLab->isWeak())
		return;
	for (auto *sLab : getConsChecker().getCoherentStores(lab)) {
		if (std::ranges::find(kept, sLab) != kept.end())
			continue;
		getExec().getWorkqueue().add(
			std::make_unique<WeakCasFailureRevisit>(lab->getPos(), sLab->getPos()));
	}
}

void GenMCDriver::maybePushWeakCasFailure(const CasReadLabel *cLab)
{
	/* Pushes the spurious-failure option of a *successful* weak CAS;
	 * callers must have already established success. */
	if (!cLab->isWeak() || cLab->failsSpuriously() || !cLab->getRf())
		return;
	getExec().getWorkqueue().add(
		std::make_unique<WeakCasFailureRevisit>(cLab->getPos(), cLab->getRf()->getPos()));
}

auto GenMCDriver::getInvalidFreeReason(SAddr loc, StorageDuration sdur) const
	-> std::optional<std::string>
{
	const auto &state = getExec().getGraph().getState();
	if (loc.isStatic())
		return "The freed address is a global, not a heap block\n";
	if (sdur == StorageDuration::SD_Heap && loc.isAutomatic())
		return "The freed address is a stack variable, not a heap block\n";
	if (!state.isAllocated(loc))
		return "The freed address is not in an allocated block\n";
	if (state.getAllocAccess(loc).addr != loc)
		return "The freed address points into a block, not at its start\n";
	return std::nullopt;
}

auto GenMCDriver::checkAccessValidity(Event pos, const AAccess &access)
	-> std::optional<VerificationError>
{
	/* Check that static accesses hit a registered global range,
	 * and dynamic accesses hit allocated memory */
	auto &state = getExec().getGraph().getState();
	if ((!getConf()->disableStaticValidityChecks && !access.addr.isDynamic() &&
	     !state.isStatic(access.addr)) ||
	    (access.addr.isDynamic() && !state.isAccessible(access.addr))) {
		reportError(pos, {pos, VerificationError::VE_AccessNonMalloc});
		return {VerificationError::VE_AccessNonMalloc};
	}
	return {};
}

auto GenMCDriver::checkInitializedMem(const ReadLabel *rLab) -> std::optional<VerificationError>
{
	// FIXME: Have label for mutex-destroy and check type instead of val.
	//        Also for barriers.

	// /* Locks should not read from destroyed mutexes */
	// const auto *lLab = genmc::dyn_cast<LockCasReadLabel>(rLab);
	// if (lLab && lLab->getAccessValue(lLab->getAccess()) == SVal(-1)) {
	// 	reportError(lLab->getPos(), {lLab->getPos(), VerificationError::VE_UninitializedMem,
	// 				     "Called lock() on destroyed mutex!", lLab->getRf()});
	// 	return {VerificationError::VE_UninitializedMem};
	// }

	/* Allow frontends to skip uninit/msa checks */
	if (getConf()->disableInitializationChecks)
		return {};

	/* Dynamic accesses should read initialized memory (if reading from INIT).
	 * We use two separate checks for atomics and NAs, since mixed-size NAs don't have an rf */
	if (rLab->getAddr().isDynamic() &&
	    (!rLab->getParent()->getState().isInitialized(rLab->getAccess()) ||
	     !getConsChecker().getHbView(rLab).contains(
		     rLab->getParent()->getState().getInitView(rLab->getAccess())))) {
		reportError(rLab->getPos(),
			    {rLab->getPos(), VerificationError::VE_UninitializedMem});
		return {VerificationError::VE_UninitializedMem};
	}

	/* Slightly unrelated check, but ensure there are no mixed-size accesses */
	if (rLab->getRf() && !rLab->getRf()->getPos().isInitializer() &&
	    genmc::dyn_cast<WriteLabel>(rLab->getRf())->getSize() != rLab->getSize()) {
		reportError(rLab->getPos(),
			    {rLab->getPos(), VerificationError::VE_MixedSize,
			     "Mixed-size accesses detected: tried to read with a " +
				     std::to_string(rLab->getSize().get() * CHAR_BIT) +
				     "-bit access!\n" + "Please check the LLVM-IR.\n"});
		return {VerificationError::VE_MixedSize};
	}
	return {};
}

auto GenMCDriver::checkInitializedMem(const WriteLabel *wLab) -> std::optional<VerificationError>
{

	/* Unlocks should unlock mutexes locked by the same thread */
	const auto *uLab = genmc::dyn_cast<UnlockWriteLabel>(wLab);
	if (uLab && !findMatchingLock(uLab)) {
		reportError(uLab->getPos(),
			    {uLab->getPos(), VerificationError::VE_InvalidUnlock,
			     "Called unlock() on mutex not locked by the same thread!"});
		return {VerificationError::VE_InvalidUnlock};
	}
	return {};
}

auto GenMCDriver::checkFinalAnnotations(const WriteLabel *wLab) -> std::optional<VerificationError>
{
	if (!getConf()->finalWrite)
		return {};

	auto &g = getExec().getGraph();

	if (!g.hasLocMoreThanOneStore(wLab->getAddr()))
		return {};
	if ((wLab->isFinal() &&
	     std::ranges::any_of(g.co(wLab->getAddr()),
				 [&](auto &sLab) {
					 return !getConsChecker().getHbView(wLab).contains(
						 sLab.getPos());
				 })) ||
	    (!wLab->isFinal() && std::ranges::any_of(g.co(wLab->getAddr()),
						     [&](auto &sLab) { return sLab.isFinal(); }))) {
		reportError(wLab->getPos(), {wLab->getPos(), VerificationError::VE_Annotation,
					     "Multiple stores at final location!"});
		return {VerificationError::VE_Annotation};
	}
	return {};
}

auto GenMCDriver::checkIPRValidity(const ReadLabel *rLab) -> std::optional<VerificationError>
{
	if (!rLab->getAnnot() || !getConf()->ipr)
		return {};

	auto &g = getExec().getGraph();
	if (!g.mayHaveWWRacyWrites())
		return {};
	auto racyIt = std::ranges::find_if(g.co(rLab->getAddr()),
					   [&](auto &wLab) { return wLab.isWWRacy(); });
	if (racyIt == std::ranges::end(g.co(rLab->getAddr())))
		return {};

	auto msg =
		"Unordered writes do not constitute a bug per se, though they often "
		"indicate faulty design.\n"
		"This warning is treated as an error due to in-place revisiting (IPR).\n"
		"You can use -disable-ipr to disable this feature."s; // NOLINT(misc-include-cleaner)
	getResult().iprError = true;
	reportError(racyIt->getPos(),
		    {racyIt->getPos(), VerificationError::VE_WWRace, msg, nullptr, true});
	return {VerificationError::VE_WWRace};
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

void GenMCDriver::filterConflictingBarriers(const ReadLabel *lab, std::vector<EventLabel *> &stores)
{
	if (getConf()->disableBAM ||
	    (!genmc::isa<BIncFaiReadLabel>(lab) && !genmc::isa<BWaitReadLabel>(lab)))
		return;

	/* Helper lambdas */
	auto isReadByExclusiveRead = [&](auto *oLab) {
		if (auto *wLab = genmc::dyn_cast<WriteLabel>(oLab))
			return std::ranges::any_of(wLab->readers(),
						   [&](auto &rLab) { return rLab.isRMW(); });
		if (auto *iLab = genmc::dyn_cast<InitLabel>(oLab))
			return std::ranges::any_of(iLab->rfs(lab->getAddr()),
						   [&](auto &rLab) { return rLab.isRMW(); });
		UNREACHABLE();
	};
	auto findFaiReader = [](BIncFaiWriteLabel *wLab) {
		return std::ranges::find_if(wLab->readers(), [](auto &rLab) {
			return genmc::isa<BIncFaiReadLabel>(&rLab);
		});
	};
	auto findSameRoundMaximal = [&](BIncFaiWriteLabel *wLab) {
		auto &g = *wLab->getParent();
		while (!isLastInBarrierRound(wLab) &&
		       findFaiReader(wLab) != std::ranges::end(wLab->readers())) {
			wLab = genmc::dyn_cast<BIncFaiWriteLabel>(
				g.po_imm_succ(&*findFaiReader(wLab)));
		}
		return wLab;
	};

	/* barrier_wait()'s plain load should read maximally */
	if (genmc::isa<BWaitReadLabel>(lab)) {
		auto *wLab = genmc::dyn_cast<BIncFaiWriteLabel>(stores[0]);
		VERIFY(wLab && wLab->getPos().next() == lab->getPos());
		stores[0] = findSameRoundMaximal(wLab);
		stores.resize(1);
		return;
	}

	/* barrier_wait()'s FAI loads should not read from conflicting stores */
	std::erase_if(stores, [&](auto &sLab) { return isReadByExclusiveRead(sLab); });
}

void GenMCDriver::filterSymmetricStoresSR(const ReadLabel *rLab,
					  std::vector<EventLabel *> &stores) const
{
	const auto &g = getExec().getGraph();
	auto tid = g.getFirstThreadLabel(rLab->getThread())->getSymmPredTid();

	/* If there is no symmetric thread, exit */
	if (tid == -1)
		return;

	/* Check whether the po-prefixes of the two threads match */
	if (!getSymmChecker().sharePrefixSR(tid, rLab))
		return;

	/* Get the symmetric event and make sure it matches as well */
	const auto *lab = genmc::dyn_cast<ReadLabel>(g.getEventLabel(Event(tid, rLab->getIndex())));
	if (!lab || lab->getAddr() != rLab->getAddr() || lab->getSize() != lab->getSize())
		return;

	if (!lab->isRMW())
		return;

	/* Remove stores that will be explored symmetrically */
	std::erase_if(stores, [&](auto &sLab) { return lab->getRf()->getPos() == sLab->getPos(); });
}

namespace {
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
	BlindRMWFilter(const ExecutionGraph &g, const ConsistencyChecker &cc) : g(g), cc(cc)
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
	const ConsistencyChecker &cc;
	std::vector<int> firstSCFence; /* -1 if the thread has none */
};
} // namespace

/* The read part of WLAB if it is an RMW write, or null */
static auto getRMWRead(const WriteLabel &wLab) -> const ReadLabel *
{
	const auto *rLab =
		genmc::dyn_cast_if_present<ReadLabel>(wLab.getParent()->po_imm_pred(&wLab));
	return rLab && rLab->isRMW() ? rLab : nullptr;
}

/*
 * Two blind relaxed FAIs that apply the same operation with the same operand
 * to the same location write the same sequence of values in either order, and
 * neither synchronizes with anything.  Executions that differ only in their
 * relative order are thus equivalent, and we keep the order in which they are
 * added: a new such FAI does not read from a store that an identical FAI
 * already reads from (which would place it before that FAI).  The co-maximal
 * store is always kept, so reads stay extensible.
 *
 * Reading co-maximally puts everything before the co-maximal store into the
 * new FAI's prefix, so later writes hb-after it can no longer revisit those
 * reads.  That loses no behavior only if (a) all writes to the location are
 * identical blind FAIs, so the only reads affected are theirs, whose revisits
 * are again swaps, and (b) no other read lies between the skipped store and the
 * co-maximal one in prefix order.  A different write to the location that
 * arrives later is reported by checkBlindRMWWellFormedness().
 */
void GenMCDriver::filterCommutingBlindRMWs(const ReadLabel *rLab,
					   std::vector<EventLabel *> &stores) const
{
	const auto &g = getExec().getGraph();
	const BlindRMWFilter blind(g, getConsChecker());
	const auto *fLab = blind.asCommuting(rLab);
	if (!fLab)
		return;

	auto addr = fLab->getAddr();

	/* (a): every write to the location so far is an identical blind FAI */
	if (!std::ranges::all_of(g.co(addr), [&](auto &wLab) {
		    const auto *pLab = getRMWRead(wLab);
		    return pLab && blind.areTwins(pLab, fLab);
	    }))
		return;

	const auto *coMax = g.co_max(addr);
	if (genmc::isa<InitLabel>(coMax))
		return;

	/* (b): between SLAB and the co-maximal store, the prefix holds only twins */
	const auto &maxView = getPrefixView(coMax);
	const auto &ownView = getPrefixView(fLab);
	auto onlyTwinsAfter = [&](const EventLabel *sLab) {
		const auto *sView = genmc::isa<InitLabel>(sLab) ? nullptr : &getPrefixView(sLab);
		for (auto t = 0; t < g.getNumThreads(); t++) {
			auto from = std::max(ownView.getMax(t), sView ? sView->getMax(t) : -1);
			for (auto i = std::max(from + 1, 0); i <= maxView.getMax(t); i++) {
				const auto *lab = g.getEventLabel(Event(t, i));
				if (genmc::isa<ReadLabel>(lab) && !blind.areTwins(lab, fLab))
					return false;
			}
		}
		return true;
	};
	auto isReadByTwin = [&](const EventLabel *sLab) {
		auto isTwin = [&](auto &oLab) { return blind.areTwins(&oLab, fLab); };
		if (const auto *wLab = genmc::dyn_cast<WriteLabel>(sLab))
			return std::ranges::any_of(wLab->readers(), isTwin);
		return std::ranges::any_of(genmc::cast<InitLabel>(sLab)->rfs(addr), isTwin);
	};

	auto kept = stores;
	std::erase_if(kept, [&](auto *sLab) {
		return sLab != coMax && isReadByTwin(sLab) && onlyTwinsAfter(sLab);
	});
	if (!kept.empty())
		stores = std::move(kept);
}

auto GenMCDriver::checkBlindRMWWellFormedness(const ReadLabel *rLab) -> bool
{
	if (!getConf()->blindRMWReduction)
		return true;

	const auto &g = getExec().getGraph();
	const BlindRMWFilter blind(g, getConsChecker());
	if (blind.asCommuting(rLab))
		return true;

	/*
	 * A read that is hb-after one blind FAI but not after an identical one
	 * observes their order: it may see the first without the second.  The
	 * reduction may have fixed that order, so such a read is not supported.
	 */
	const auto &view = getConsChecker().getHbView(rLab);
	std::vector<const ReadLabel *> twins;
	for (const auto &wLab : g.co(rLab->getAddr())) {
		if (const auto *pLab = getRMWRead(wLab); blind.asCommuting(pLab))
			twins.push_back(pLab);
	}
	auto valid = std::ranges::none_of(twins, [&](auto *aLab) {
		return view.contains(aLab->getPos()) && std::ranges::any_of(twins, [&](auto *bLab) {
			       return blind.areTwins(aLab, bLab) && !view.contains(bLab->getPos());
		       });
	});
	if (!valid)
		reportError(
			rLab->getPos(),
			{rLab->getPos(), VerificationError::VE_BlindRMWWellFormedness,
			 "This read observes the order of blind RMWs that --blind-rmw-reduction "
			 "treats as unordered. Rerun without --blind-rmw-reduction.\n"});
	return valid;
}

auto GenMCDriver::checkBlindRMWWellFormedness(const WriteLabel *sLab) -> bool
{
	if (!getConf()->blindRMWReduction)
		return true;

	/*
	 * The reduction assumed that identical blind FAIs are the only writes to
	 * their location.  If another write is not hb-after one of them, an order
	 * we skipped might have let it revisit that FAI.  (Its prefix is no guide:
	 * the order we kept may have put the FAIs there through their rf chain,
	 * which creates no hb, as they are relaxed.)
	 */
	const auto &g = getExec().getGraph();
	const BlindRMWFilter blind(g, getConsChecker());
	const auto *ownRead = getRMWRead(*sLab);
	const auto &view = getConsChecker().getHbView(sLab);
	std::vector<const ReadLabel *> twins;
	for (const auto &wLab : g.co(sLab->getAddr())) {
		const auto *rLab = getRMWRead(wLab);
		if (&wLab != sLab && blind.asCommuting(rLab) && !blind.areTwins(rLab, ownRead))
			twins.push_back(rLab);
	}
	auto valid = std::ranges::none_of(twins, [&](auto *aLab) {
		return !view.contains(aLab->getPos()) &&
		       std::ranges::any_of(twins,
					   [&](auto *bLab) { return blind.areTwins(aLab, bLab); });
	});
	if (!valid)
		reportError(
			sLab->getPos(),
			{sLab->getPos(), VerificationError::VE_BlindRMWWellFormedness,
			 "Blind RMWs on this location are reordered by --blind-rmw-reduction, but "
			 "this write is not one of them. Rerun without --blind-rmw-reduction.\n"});
	return valid;
}

void GenMCDriver::filterValuesFromAnnotSAVER(const ReadLabel *rLab,
					     std::vector<EventLabel *> &validStores)
{
	/* Locks are treated as annotated CASes */
	if (!rLab->getAnnot())
		return;

	using Evaluator = SExprEvaluator<ModuleVarID>;

	auto &g = getExec().getGraph();

	/* Ensure we keep the maximal store around even if Helper messed with it */
	VERIFY(!validStores.empty());
	auto *maximal = validStores.back();
	std::erase_if(validStores, [&](auto *wLab) {
		auto val = wLab->getAccessValue(rLab->getAccess());
		return wLab != maximal && wLab != g.co_max(rLab->getAddr()) &&
		       !Evaluator().evaluate(&*rLab->getAnnot()->expr, val);
	});
	VERIFY(!validStores.empty());
}

void GenMCDriver::unblockWaitingHelping(const WriteLabel *lab)
{
	if (!genmc::isa<HelpedCasWriteLabel>(lab))
		return;

	/* FIXME: We have to wake up all threads waiting on helping CASes,
	 * as we don't know which ones are from the same CAS */
	for (auto i = 0; i < getExec().getGraph().getNumThreads(); i++) {
		auto *bLab = genmc::dyn_cast_if_present<HelpedCASBlockLabel>(
			getExec().getGraph().getLastThreadLabel(i));
		if (bLab)
			getExec().getGraph().removeLast(bLab->getThread());
	}
}

auto GenMCDriver::writesBeforeHelpedContainedInView(const HelpedCasReadLabel *lab, const View &view)
	-> bool
{
	auto &g = getExec().getGraph();
	const auto &hb = getConsChecker().getHbView(lab);

	for (auto i = 0; i < hb.size(); i++) {
		auto j = hb.getMax(i);
		while (!genmc::isa<WriteLabel>(g.getEventLabel(Event(i, j))) && j > 0)
			--j;
		if (j > 0 && !view.contains(Event(i, j)))
			return false;
	}
	return true;
}

auto GenMCDriver::checkHelpingCasCondition(const HelpingCasLabel *hLab) -> bool
{
	auto &g = getExec().getGraph();

	auto hsView = g.labels() | std::views::filter([hLab](auto &lab) {
			      auto *rLab = genmc::dyn_cast<HelpedCasReadLabel>(&lab);
			      return rLab && rLab->isRMW() && rLab->getAddr() == hLab->getAddr() &&
				     rLab->getSize() == hLab->getSize() &&
				     rLab->getOrdering() == hLab->getOrdering() &&
				     rLab->getExpected() == hLab->getExpected() &&
				     rLab->getSwapVal() == hLab->getSwapVal();
		      });

	if (std::ranges::any_of(hsView, [this](auto &lab) {
		    auto *hLab = genmc::dyn_cast<HelpedCasReadLabel>(&lab);
		    auto &view = getConsChecker().getHbView(hLab);
		    return !writesBeforeHelpedContainedInView(hLab, view);
	    }))
		ERROR("Helped/Helping CAS annotation error! "
		      "Not all stores before helped-CAS are visible to helping-CAS!");
	return std::ranges::begin(hsView) != std::ranges::end(hsView);
}

auto GenMCDriver::findConsistentRf(ReadLabel *rLab, std::vector<EventLabel *> &rfs) -> EventLabel *
{

	/* Otherwise, search for a consistent rf */
	std::ranges::reverse(rfs);
	while (rfs.size() > 1) {
		auto *back = rfs.back();
		rfs.pop_back();
		rLab->setRf(back);
		if (isExecutionValid(rLab)) {
			updateLabelViews(rLab);
			return back;
		}
	}

	auto *back = rfs.back();
	rfs.pop_back();
	rLab->setRf(back);
	updateLabelViews(rLab);

	/* For the non-bounding case, maximal extensibility guarantees consistency */
	if (!getConf()->bound.has_value() || isExecutionValid(rLab))
		return back;

	/* Extensibility is guaranteed with bounding because
	 * - the consistent choice might lead to a settled atomicity violation and has already been
	 * filtered-out
	 * - context bounding's slack might have changed due to an optimization */
	VERIFY(getConf()->bound.has_value() &&
	       (getConf()->boundType == BoundType::context || genmc::isa<CasReadLabel>(rLab) ||
		genmc::isa<FaiReadLabel>(rLab)));
	return nullptr;
}

auto GenMCDriver::isPlacementConsistent(const WriteLabel *wLab) const -> bool
{
	/* A write placed last in coherence keeps a consistent graph consistent
	 * (extensibility), which findConsistentCo() relies on. The write of an
	 * RMW goes right after the write its read part read, which need not be
	 * the last one; under LKMM its coe edges then start prop, and can close
	 * an hb cycle (R ->ctrl W ->coe W' ->po-rel W'' ->rfe R) that the
	 * checks of the read part could not see */
	const auto &g = *wLab->getParent();
	return getConf()->model != ModelType::LKMM || g.co_max(wLab->getAddr()) == wLab ||
	       getConsChecker().isConsistent(wLab);
}

auto GenMCDriver::findConsistentCo(WriteLabel *wLab, std::vector<EventLabel *> &cos) -> EventLabel *
{

	/* Similarly to the read case: rely on extensibility */
	auto *back = cos.back();
	wLab->addCo(back);
	if (!getConf()->bound.has_value()) {
		cos.pop_back();
		return back;
	}

	// FIXME: This is wrong
	/* In contrast to the read case, we need to be a bit more careful:
	 * the consistent choice might not satisfy atomicity, but we should
	 * keep it around to try revisits */
	while (!cos.empty()) {
		auto *back = cos.back();
		cos.pop_back();
		wLab->moveCo(back);
		if (isExecutionValid(wLab))
			return back;
	}
	return nullptr;
}

auto GenMCDriver::handleThreadKill(std::unique_ptr<ThreadKillLabel> kLab)
	-> HandleResult<std::monostate>
{
	addLabelToGraph(std::move(kLab));
	return {.result = std::monostate{}, .count = 1U};
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

auto GenMCDriver::isSymmetricToSR(int candidate, Event parent, const ThreadInfo &info) const -> bool
{
	const auto &g = getExec().getGraph();
	auto cParent = g.getFirstThreadLabel(candidate)->getCreateId();
	const auto &cInfo = g.getFirstThreadLabel(candidate)->getThreadInfo();

	/* A tip to print to the user in case two threads look
	 * symmetric, but we cannot deem it */
	auto tipSymmetry = [&]() {
		LOG_ONCE("possible-symmetry", VerbosityLevel::Tip,
			 "Threads {} and {} could benefit from symmetry reduction. Consider using "
			 "__VERIFIER_spawn_symmetric().\n",
			 cInfo.id, info.id);
	};

	/* First, check that the two threads are actually similar (the arguments
	 * only matter if the thread body reads its argument) */
	if (cInfo.id == info.id || cInfo.parentId != info.parentId || cInfo.funId != info.funId ||
	    (info.argUsed && cInfo.arg != info.arg)) {
		if (cInfo.funId == info.funId && cInfo.parentId == info.parentId)
			tipSymmetry();
		return false;
	}

	/* Then make sure that there is no memory access in between the spawn
	 * events, other than to memory only the parent can access (e.g.,
	 * storing the first thread's handle into a local array) */
	auto bounds = std::minmax(parent.index, cParent.index);
	auto lastMemAccess = g.getState().getLastMemAccess(parent.thread);
	if (lastMemAccess > bounds.first && lastMemAccess <= bounds.second &&
	    !areThreadPrivateAccesses(parent.thread, bounds.first + 1, lastMemAccess)) {
		tipSymmetry();
		return false;
	}
	return true;
}

auto GenMCDriver::areThreadPrivateAccesses(int tid, int from, int to) const -> bool
{
	/* Without labels for non-atomic accesses, they cannot be told apart */
	if constexpr (!Config::emitNALabels)
		return false;

	const auto &g = getExec().getGraph();
	const auto &state = g.getState();
	for (auto i = from; i <= to; i++) {
		const auto *mLab = genmc::dyn_cast<MemAccessLabel>(g.getEventLabel(Event(tid, i)));
		if (!mLab)
			continue;
		if (!state.isAllocated(mLab->getAddr()))
			return false;
		const auto *aLab = genmc::dyn_cast_if_present<MallocLabel>(
			g.getEventLabel(state.getAllocPos(mLab->getAddr())));
		if (!aLab || !aLab->isThreadPrivate() || aLab->getThread() != tid)
			return false;
	}
	return true;
}

auto GenMCDriver::getSymmetricTidSR(const ThreadCreateLabel *tcLab,
				    const ThreadInfo &childInfo) const -> int
{
	if (!getConf()->symmetryReduction)
		return -1;

	const auto &g = getExec().getGraph();
	auto isOrphan = [&g](int tid) { return !g.getFirstThreadLabel(tid)->getCreate(); };

	/* Has the user provided any info? (A thread whose create was cut has a
	 * stale begin and cannot be a symmetric predecessor) */
	if (childInfo.symmId != -1)
		return isOrphan(childInfo.symmId) ? -1 : childInfo.symmId;

	for (auto i = childInfo.id - 1; i > 0; i--)
		if (!isOrphan(i) && isSymmetricToSR(i, tcLab->getPos(), childInfo))
			return i;
	return -1;
}

auto GenMCDriver::handleThreadCreate(std::unique_ptr<ThreadCreateLabel> tcLab) -> HandleResult<int>
{
	auto &g = getExec().getGraph();

	/* First, check if the thread to be created already exists: a restriction
	 * may have cut the create but kept the (terminated) begin of a thread
	 * that could not be removed (see ExecutionGraph::copyGraphUpTo()) */
	int cid = 0;
	while (std::cmp_less(cid, g.getNumThreads())) {
		if (!g.isThreadEmpty(cid)) {
			auto *bLab = genmc::dyn_cast_if_present<ThreadStartLabel>(
				g.getFirstThreadLabel(cid));
			if (bLab && bLab->getCreateId() == tcLab->getPos())
				break;
		}
		++cid;
	}

	/* Add an event for the thread creation */
	tcLab->setChildId(cid);
	auto *lab = genmc::dyn_cast<ThreadCreateLabel>(addLabelToGraph(std::move(tcLab)));
	g.getState().onThreadCreate(lab->getPos());

	if (std::cmp_less(cid, g.getNumThreads())) {
		/* Revive the thread: replace its stale begin (the thread info may differ) */
		auto *bLab = g.getFirstThreadLabel(cid);
		VERIFY(g.getThreadSize(cid) == 1 && !bLab->getCreate() && bLab->isTerminated());
		if (auto *predLab = bLab->getSymmPredTid() != -1
					    ? g.getFirstThreadLabel(bLab->getSymmPredTid())
					    : nullptr;
		    predLab && predLab->getSymmSuccTid() == cid)
			predLab->setSymmSuccTid(-1);
		g.removeLast(cid);
	} else {
		g.addNewThread();
	}

	/* Create a label and add it to the graph; is the thread symmetric to another one? */
	auto symm = getSymmetricTidSR(lab, lab->getChildInfo());
	addLabelToGraph(ThreadStartLabel::create(Event(cid, 0), lab->getPos(), lab,
						 lab->getChildInfo(), symm));
	if (symm != -1)
		g.getFirstThreadLabel(symm)->setSymmSuccTid(cid);

	/* Unordered writes are an error under SR once the graph has symmetric
	 * threads (see reportWarningOnce()), also when they were found first:
	 * which comes first depends on the schedule (#130) */
	if (symm != -1 && getResult().warnings.contains(VerificationError::VE_WWRace)) {
		getResult().inferredSymmetryError =
			lab->getChildInfo().symmId == -1 &&
			std::ranges::none_of(g.thr_ids(), [&](auto tid) {
				const auto *bLab = g.getFirstThreadLabel(tid);
				return bLab->getSymmPredTid() != -1 &&
				       bLab->getThreadInfo().symmId != -1;
			});
		auto msg = std::format(
			"Thread {} is symmetric to thread {}, and the program has unordered "
			"writes.\n"
			"Unordered writes do not constitute a bug per se, though they often "
			"indicate faulty design.\n"
			"This warning is treated as an error due to symmetry reduction (SR).\n"
			"You can use -disable-sr to disable it.",
			cid, symm);
		reportError(lab->getPos(), {lab->getPos(), VerificationError::VE_WWRace, msg});
		return {.result = VerificationError::VE_WWRace};
	}

	return {.result = cid, .count = 1U};
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

auto GenMCDriver::checkSymmetricJoinOrder(const ThreadJoinLabel *jLab)
	-> std::optional<VerificationError>
{
	/* Symmetry reduction explores symmetric threads as if each one ran no
	 * later than its symmetric successors. A thread that joins a symmetric
	 * thread before its symmetric predecessors can tell them apart: the
	 * predecessors may still be running, which the reduced exploration
	 * misses (e.g., a race with an unjoined predecessor) */
	const auto &g = getExec().getGraph();
	auto cid = jLab->getChildId();
	if (cid < 0 || long(g.getNumThreads()) <= cid)
		return std::nullopt;

	/* The return value is another way to tell symmetric threads apart */
	const auto *cLab = g.getFirstThreadLabel(cid);
	auto pred = cLab->getSymmPredTid();
	auto symmetric = pred != -1 || cLab->getSymmSuccTid() != -1;
	auto resultUsed = symmetric && jLab->isResultUsed();

	/* Checking the immediate predecessor suffices: its own join was checked
	 * against its predecessor when it was added */
	auto predJoined = [&] {
		const auto *eLab =
			genmc::dyn_cast_if_present<ThreadFinishLabel>(g.getLastThreadLabel(pred));
		return eLab && eLab->getParentJoin() &&
		       eLab->getParentJoin()->getThread() == jLab->getThread();
	};
	if (!resultUsed && (pred == -1 || predJoined()))
		return std::nullopt;

	auto inferred = true;
	for (auto tid = cid; tid != -1; tid = g.getFirstThreadLabel(tid)->getSymmPredTid())
		inferred &= g.getFirstThreadLabel(tid)->getThreadInfo().symmId == -1;
	/* (The successors of a symmetric thread may have inferred symmetry) */
	for (auto tid = cLab->getSymmSuccTid(); tid != -1;
	     tid = g.getFirstThreadLabel(tid)->getSymmSuccTid())
		inferred &= g.getFirstThreadLabel(tid)->getThreadInfo().symmId == -1;
	getResult().inferredSymmetryError = inferred;
	auto msg = std::format(
		"{}\nSymmetry reduction requires joining symmetric threads in the order they were "
		"spawned, without using their return values. You can use -disable-sr to "
		"disable it.\n",
		resultUsed ? std::format("The return value of symmetric thread {} is used.", cid)
			   : std::format("Thread {} is joined before its symmetric predecessors.",
					 cid));
	reportError(jLab->getPos(),
		    {jLab->getPos(), VerificationError::VE_SymmetryWellFormedness, msg});
	return VerificationError::VE_SymmetryWellFormedness;
}

auto GenMCDriver::handleThreadJoin(std::unique_ptr<ThreadJoinLabel> lab) -> HandleResult<SVal>
{
	auto &g = getExec().getGraph();

	if (!genmc::isa_and_present<ThreadFinishLabel>(g.getLastThreadLabel(lab->getChildId()))) {
		blockThread(g, JoinBlockLabel::create(lab->getPos(), lab->getChildId()));
		return {.result = Reset{}, .count = 0U};
	}

	auto *jLab = genmc::dyn_cast<ThreadJoinLabel>(addLabelToGraph(std::move(lab)));
	auto cid = jLab->getChildId();

	if (auto err = checkSymmetricJoinOrder(jLab); err)
		return {.result = *err};

	auto *eLab = genmc::dyn_cast<ThreadFinishLabel>(g.getLastThreadLabel(cid));
	VERIFY(eLab);
	eLab->setParentJoin(jLab);

	if (cid < 0 || long(g.getNumThreads()) <= cid || std::cmp_equal(cid, jLab->getThread())) {
		std::string err = "ERROR: Invalid TID in pthread_join(): " + std::to_string(cid);
		if (std::cmp_equal(cid, jLab->getThread()))
			err += " (TID cannot be the same as the calling thread)";
		reportError(jLab->getPos(),
			    {jLab->getPos(), VerificationError::VE_InvalidJoin, err});
		return {.result = VerificationError::VE_InvalidJoin};
	}

	if (partialExecutionExceedsBound()) {
		moot();
		return {.result = Invalid{}, .count = 1U};
	}

	return {.result = jLab->getReturnValue(), .count = 1U};
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

/* The depth of the RCU read-side critical sections open in POS's thread
 * before POS (an rcu_read_unlock() without a lock leaves it at zero) */
static auto getRcuNesting(const ExecutionGraph &g, Event pos) -> unsigned
{
	auto nesting = 0U;
	for (auto i = 0; i < pos.index; i++) {
		const auto *fLab =
			genmc::dyn_cast_if_present<LkmmFenceLabel>(g.getEventLabelIfPresent(Event(pos.thread, i)));
		if (fLab && fLab->getFenceKind() == LkmmFenceKind::RcuLock)
			++nesting;
		else if (fLab && fLab->getFenceKind() == LkmmFenceKind::RcuUnlock && nesting > 0)
			--nesting;
	}
	return nesting;
}

auto GenMCDriver::handleThreadFinish(std::unique_ptr<ThreadFinishLabel> eLab)
	-> HandleResult<std::monostate>
{
	auto &g = getExec().getGraph();

	/* The run was dropped while aligning this event (alignKeptEvents()) */
	if (droppedBeforeKept_)
		return {.result = Reset{}, .count = 0U};

	auto *lab = addLabelToGraph(std::move(eLab));

	/* Kept events after the end (see alignKeptEvents()) are never reached */
	if (relocatesKeptEvents() &&
	    std::ranges::any_of(std::views::iota(lab->getIndex() + 1,
						 g.getThreadSize(lab->getThread())),
				[&](int i) {
					return !genmc::isa<EmptyLabel>(
						g.getEventLabel(Event(lab->getThread(), i)));
				})) {
		++result.relocationsLost;
		moot();
		return {.result = std::monostate(), .count = 1U};
	}
	if (getRcuNesting(g, lab->getPos()) > 0) {
		reportError(lab->getPos(),
			    {lab->getPos(), VerificationError::VE_InvalidRcu,
			     "Thread ends in an RCU read-side critical section!"});
		return {.result = VerificationError::VE_InvalidRcu};
	}
	if (auto err = checkEndedThreadLocals(genmc::cast<ThreadFinishLabel>(lab)); err)
		return {.result = *err};
	for (auto i = 0; i < g.getNumThreads(); i++) {
		auto *pLab = genmc::dyn_cast_if_present<JoinBlockLabel>(g.getLastThreadLabel(i));
		if (pLab && std::cmp_equal(pLab->getChildId(), lab->getThread())) {
			/* If parent thread is waiting for me, relieve it */
			unblockThread(g, pLab->getPos());
		}
	}
	if (partialExecutionExceedsBound())
		moot();
	return {.result = std::monostate(), .count = 1U};
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

auto GenMCDriver::handleFence(std::unique_ptr<FenceLabel> fLab) -> HandleResult<std::monostate>
{
	auto *lab = addLabelToGraph(std::move(fLab));
	getExec().getGraph().getState().onFence(lab->getPos(), lab->isAtLeastRelease());
	return {.result = std::monostate(), .count = 1U};
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

void GenMCDriver::checkReconsiderFaiSpinloop(const MemAccessLabel *lab)
{
	auto &g = getExec().getGraph();

	for (auto i = 0; i < g.getNumThreads(); i++) {
		/* Is there any thread blocked on a potential spinloop? */
		auto *eLab = genmc::dyn_cast_if_present<FaiZNEBlockLabel>(g.getLastThreadLabel(i));
		if (!eLab)
			continue;

		/* Check whether this access affects the spinloop variable */
		auto epreds = g.po_preds(eLab);
		auto faiLabIt = std::ranges::find_if(
			epreds, [](auto &lab) { return genmc::isa<FaiWriteLabel>(&lab); });
		VERIFY(faiLabIt != std::ranges::end(epreds));

		auto *faiLab = genmc::dyn_cast<FaiWriteLabel>(&*faiLabIt);
		if (faiLab->getAddr() != lab->getAddr())
			continue;

		/* FAIs on the same variable are OK... */
		if (genmc::isa<FaiReadLabel>(lab) || genmc::isa<FaiWriteLabel>(lab))
			continue;

		/* If it does, and also breaks the assumptions, unblock thread */
		if (!getConsChecker().getHbView(faiLab).contains(lab->getPos())) {
			auto pos = eLab->getPos();
			auto key = eLab->getSharedKey();
			auto keySub = eLab->getKeySub();
			unblockThread(g, pos);
			auto zLab = FaiZNESpinEndLabel::create(pos);
			zLab->setKey(std::move(key), keySub);
			addLabelToGraph(std::move(zLab));
		}
	}
}

auto GenMCDriver::getPrefixView(const EventLabel *lab) const -> const VectorClock &
{
	if (!lab->hasPrefixView())
		lab->setPrefixView(getConsChecker().calculatePrefixView(lab));
	return lab->getPrefixView();
}

auto GenMCDriver::getRevisitPrefixView(const EventLabel *lab) const
	-> std::unique_ptr<VectorClock>
{
	if (!scopesCtrlDeps())
		return getPrefixView(lab).clone();
	return getConsChecker().calculateRevisitPrefixView(lab);
}

auto GenMCDriver::getRfsApproximation(ReadLabel *lab) -> std::vector<EventLabel *>
{
	auto &checker = getConsChecker();
	auto rfs = checker.getCoherentStores(lab);
	if (!genmc::isa<CasReadLabel>(lab) && !genmc::isa<FaiReadLabel>(lab))
		return rfs;

	/* Remove atomicity violations */
	const auto &before = getPrefixView(lab);
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

void GenMCDriver::filterOptimizeRfs(const ReadLabel *lab, std::vector<EventLabel *> &stores)
{
	/* Symmetry reduction */
	if (getConf()->symmetryReduction)
		filterSymmetricStoresSR(lab, stores);

	/* BAM */
	if (!getConf()->disableBAM)
		filterConflictingBarriers(lab, stores);

	/* Identical blind RMWs commute */
	if (getConf()->blindRMWReduction)
		filterCommutingBlindRMWs(lab, stores);

	/* Keep values that do not lead to blocking (-exhaustive explores the
	 * blocked executions too: the annotations are not trusted) */
	if (!getConf()->exhaustive)
		filterValuesFromAnnotSAVER(lab, stores);
}

void GenMCDriver::filterAtomicityViolations(const ReadLabel *rLab,
					    std::vector<EventLabel *> &stores)
{
	auto &g = getExec().getGraph();
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

auto GenMCDriver::pickRandomRf(ReadLabel *rLab, std::vector<EventLabel *> &stores) -> EventLabel *
{

	std::erase_if(stores, [&](auto &sLab) {
		rLab->setRf(sLab);
		return !isExecutionValid(rLab);
	});

	/* There is no bounding during estimation; reads are always extensible */
	VERIFY(!stores.empty());

	MyDist dist(0, stores.size() - 1);
	auto random = dist(estRng);
	rLab->setRf(stores[random]);
	updateLabelViews(rLab);
	return stores[random];
}

auto GenMCDriver::checkForMixedSize(MemAccessLabel *lab) -> std::optional<VerificationError>
{
	auto *g = lab->getParent();

	if (lab->isNotAtomic())
		return {};

	if (g->getState().isAtomicAccessConsistent(lab->getAccess()))
		return {};

	reportError(lab->getPos(),
		    {lab->getPos(), VerificationError::VE_MixedSize,
		     "Mixed-size accesses detected: tried to read with a " +
			     std::to_string(lab->getSize().get() * CHAR_BIT) + "-bit access!\n"});
	return {VerificationError::VE_MixedSize};
}

auto GenMCDriver::handleLoad(std::unique_ptr<ReadLabel> rLab, std::optional<SVal> oldVal)
	-> HandleResult<SVal>
{
	auto &g = getExec().getGraph();
	auto &frontier = g.getState();

	/* The run was dropped while aligning this event (alignKeptEvents()) */
	if (droppedBeforeKept_)
		return {.result = Reset{}, .count = 0U};

	auto *lab = genmc::dyn_cast<ReadLabel>(addLabelToGraph(std::move(rLab)));
	auto err = checkAccessValidity(lab->getPos(), lab->getAccess())
			   .or_else([&] { return checkInitializedMem(lab); })
			   .or_else([&] { return checkForMixedSize(lab); })
			   .or_else([&] { return checkIPRValidity(lab); })
			   .or_else([&] { return checkForRaces(lab); });
	if (err)
		return {.result = *err}; /* This execution will be blocked */

	VERIFY(!lab->isNotAtomic() || exploresPlainAccesses());
	/* Runtime sends the NA value lazily when adding next atomic access */
	if (oldVal)
		g.updateDeferredValue(lab->getAccess(), *oldVal);
	if (lab->isNotAtomic())
		frontier.onNALoad(lab->getPos(), lab->getAccess(), getConsChecker().getHbView(lab));
	else
		frontier.onATLoad(lab->getPos(), lab->getAccess(), getConsChecker().getHbView(lab));

	if (!checkBlindRMWWellFormedness(lab))
		return {.result = VerificationError::VE_BlindRMWWellFormedness};

	/* Check whether the load forces us to reconsider some existing event */
	checkReconsiderFaiSpinloop(lab);

	/* If a CAS read cannot be added maximally, reschedule */
	if (!getScheduler().isRescheduledRead(lab->getPos()) &&
	    removeCASReadIfBlocks(lab, g.co_max(lab->getAddr())))
		return {.result = Reset{}, .count = 0U};
	if (getScheduler().isRescheduledRead(lab->getPos()))
		getScheduler().setRescheduledRead(Event::getInit());

	/* Get an approximation of the stores we can read from */
	auto stores = getRfsApproximation(lab);
	/* -exhaustive under IMM: the read fills a hole, and the events a revisit
	 * kept after it leave it no coherent write (see getCoherentStores()).
	 * It reads from the last write, for the graph to stay well formed until
	 * the run is dropped */
	if (stores.empty() && getConf()->exhaustive && getConf()->model == ModelType::IMM) {
		repairRead(g, lab);
		moot();
		return {.result = Invalid{}, .count = 1U};
	}
	VERIFY(!stores.empty());
	GENMC_DEBUG(LOG(VerbosityLevel::Debug3, "Rfs: {}", stores););
	if (inVerificationMode())
		pushWeakCasFailuresOnDroppedRfs(lab, stores);
	filterOptimizeRfs(lab, stores);
	GENMC_DEBUG(LOG(VerbosityLevel::Debug3, "Rfs (optimized): {}", stores););

	const EventLabel *rf = nullptr;

	if (inEstimationMode() || inRandomMode()) {
		if (inEstimationMode())
			getExec().getChoiceMap().update(lab, stores);
		filterAtomicityViolations(lab, stores);
		rf = pickRandomRf(lab, stores);
	} else {
		rf = findConsistentRf(lab, stores);
		/* Push all the other alternatives choices to the Stack */
		auto rp = stores.empty() ? nullptr : saveResumePoint(lab->getPos());
		for (const auto &sLab : stores) {
			auto fr = std::make_unique<ReadForwardRevisit>(
				lab->getPos(), sLab->getPos(), sLab == g.co_max(lab->getAddr()));
			if (rp) {
				fr->setResumePoint(rp);
				++rp->pending;
			}
			getExec().getWorkqueue().add(std::move(fr));
		}
	}

	if (!rf) {
		moot();
		return {.result = Invalid{}, .count = 1U};
	}

	/* The read's write part may have been kept while the read is executed
	 * anew (see completeRevisitedRMW()): it follows the read's rf */
	if (lab->isRMW())
		updateKeptRMWWrite(lab);

	GENMC_DEBUG(LOG(VerbosityLevel::Debug2, "--- Added load {}\n{}", lab->getPos(),
			getExec().getGraph()););

	/* Return the appropriate value. For weak CASes, also explore an execution
	 * where the CAS spuriously fails. */
	auto res = getReadRetValue(lab);
	if (const auto *cLab = genmc::dyn_cast<CasReadLabel>(lab);
	    cLab && cLab->isWeak() && !inEstimationMode() && !inRandomMode()) {
		if (const auto *v = std::get_if<SVal>(&res.result);
		    v && cLab->valueMakesRMWSucceed(*v))
			maybePushWeakCasFailure(cLab);
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

auto GenMCDriver::handleNALoad(Event pos, SAddr loc, ASize size, const EventDeps &deps)
	-> GenMCDriver::NALoadResult
{
	auto &g = getExec().getGraph();

#if EMIT_NA_LABELS
	/* Without the DRF guarantee (LKMM), or without its race reports
	 * (-disable-race-detection), a plain read may read any write the model
	 * allows, so it is explored like a marked one */
	if (exploresPlainAccesses())
		return handleLoad(ReadLabel::create(pos, MemOrdering::NotAtomic, loc, size, deps),
				  std::nullopt);
#endif
	if (!readProbeLab_)
		readProbeLab_ = std::make_unique<ReadLabel>(pos, MemOrdering::NotAtomic, loc, size);
	configureProbe(readProbeLab_.get(), pos, loc, size, deps);

	/* Cache the event before updating views (inits are added w/ tcreate) */
	if (getConf()->instructionCaching && inVerificationMode())
		getScheduler().cacheEventLabel(g, readProbeLab_.get());

	auto guard = g.addScoped(readProbeLab_);
	auto *lab = guard.get();

	/* Update views */
	updateLabelViews(lab);

	auto err = checkAccessValidity(lab->getPos(), lab->getAccess())
			   .or_else([&] { return checkInitializedMem(lab); })
			   .or_else([&] { return checkForRaces(lab); });
	if (err) {
		guard.commit();
		return {*err};
	}

	g.getState().onNALoad(pos, {loc, size}, getConsChecker().getHbView(lab));
#if EMIT_NA_LABELS
	lab->setRf(g.co_max(lab->getAddr()));
	guard.commit();
	return getReadRetValue(lab);
#else
	return {std::monostate(), 0U};
#endif
}

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

auto GenMCDriver::handleNAStore(Event pos, SAddr loc, ASize size, std::optional<SVal> val,
				const EventDeps &deps) -> GenMCDriver::NAStoreResult
{
	auto &g = getExec().getGraph();

#if EMIT_NA_LABELS
	/* Plain writes then take any place in coherence the model allows */
	if (exploresPlainAccesses())
		return handleStore(WriteLabel::create(pos, MemOrdering::NotAtomic, loc, size, *val,
						      WriteAttr::None, deps),
				   std::nullopt);
#endif
	if (!writeProbeLab_)
		writeProbeLab_ = std::make_unique<WriteLabel>(pos, MemOrdering::NotAtomic, loc,
							      size, SVal(0));
	configureProbe(writeProbeLab_.get(), pos, loc, size, deps);
	if (val)
		writeProbeLab_->setVal(*val);

	/* Cache the event before updating views (inits are added w/ tcreate) */
	if (getConf()->instructionCaching && inVerificationMode())
		getScheduler().cacheEventLabel(g, writeProbeLab_.get());

	auto guard = g.addScoped(writeProbeLab_);
	auto *lab = guard.get();

	updateLabelViews(lab);

	auto err = checkAccessValidity(lab->getPos(), lab->getAccess()).or_else([&] {
		return checkForRaces(lab);
	});
	if (err) {
		guard.commit();
		return {*err};
	}

	g.getState().onNAStore(pos, {loc, size}, getConsChecker().getHbView(lab), val);
#if EMIT_NA_LABELS
	lab->addCo(g.co_max(lab->getAddr()));
	guard.commit();
	return {true, 1U};
#else
	return {std::monostate(), 0U};
#endif
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

/* The reads SLAB may revisit: same-location reads outside its prefix BEFORE.
 * TruSt skips the stable ones (in a revisit's prefix, or not reading
 * maximally: their revisits are explored from the maximal path), and the
 * reads added after an RMW that conflicts with SLAB. With ALL (-exhaustive),
 * every such read is a candidate, and GenMCDriver::keepsConflictingRMW()
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

	/* Fastpath: previous co-max is ppo-before SLAB */
	auto prevCoMaxIt = std::ranges::find_if(
		g.rco(sLab->getAddr()), [&](auto &lab) { return lab.getPos() != sLab->getPos(); });
	if (prevCoMaxIt != std::ranges::end(g.rco(sLab->getAddr())) &&
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
 * revisit drops its write part (getRevisitView()), and the consistency of
 * the graph it produces decides (backwardRevisit()) */
static auto failsReading(const ReadLabel *rLab, const WriteLabel *sLab) -> bool
{
	const auto *cLab = genmc::dyn_cast<CasReadLabel>(rLab);
	return cLab && cLab->isRMW() &&
	       !cLab->valueMakesRMWSucceed(sLab->getAccessValue(cLab->getAccess()));
}

/* Whether reading from SLAB changes the ordering or the write of the CAS RLAB */
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

void GenMCDriver::addRevisitableCases(WriteLabel *sLab, const VectorClock &prefix,
				       std::vector<ReadLabel *> &loads)
{
	for (auto &lab : getExec().getGraph().labels()) {
		auto *rLab = genmc::dyn_cast<ReadLabel>(&lab);
		if (rLab && rLab->getAddr() == sLab->getAddr() && prefix.contains(rLab->getPos()) &&
		    rLab->getThread() != sLab->getThread() && rLab->getRf() != sLab &&
		    outcomeChanges(rLab, sLab))
			loads.push_back(rLab);
	}
}

auto GenMCDriver::getRevisitableApproximation(WriteLabel *sLab) -> std::vector<ReadLabel *>
{
	/* Without a read of the location, there is nothing to revisit, and the
	 * prefix, costly under the dependency-tracking models, waits until it
	 * is needed (the other models' checkers take a read's prefix from its
	 * write's, so they keep computing it here) */
	if (getConf()->isDepTrackingModel && !hasReads(getExec().getGraph(), sLab->getAddr()))
		return {};

	/* (Computed for the other models' checkers, see above) */
	getPrefixView(sLab);
	auto prefix = getRevisitPrefixView(sLab);
	auto loads = getRevisitable(sLab, *prefix, getConf()->exhaustive);
	getConsChecker().filterCoherentRevisits(sLab, loads);
	if (getConf()->exhaustive && getConf()->model == ModelType::IMM)
		addRevisitableCases(sLab, *prefix, loads);
	std::ranges::sort(
		loads, [](auto &lab1, auto &lab2) { return lab1->getStamp() > lab2->getStamp(); });
	return loads;
}

auto GenMCDriver::pickRandomCo(WriteLabel *sLab, std::vector<EventLabel *> &cos) -> EventLabel *
{

	sLab->addCo(cos.back());
	cos.erase(std::remove_if(cos.begin(), cos.end() - 1,
				 [&](auto &wLab) {
					 sLab->moveCo(wLab);
					 return !isExecutionValid(sLab);
				 }),
		  cos.end() - 1);

	/* Extensibility is not guaranteed if an RMW read is not reading maximally
	 * (during estimation, reads read from arbitrary places anyway).
	 * If that is the case, we have to ensure that estimation won't stop. */
	if (cos.empty()) {
		VERIFY(sLab->isRMW());
		if (inEstimationMode())
			getExec().getWorkqueue().add(std::make_unique<RerunForwardRevisit>());
		else
			moot(); /* random mode: waste the sample and resample */
		return nullptr;
	}

	MyDist dist(0, cos.size() - 1);
	auto random = dist(estRng);
	sLab->moveCo(cos[random]);
	return cos[random];
}

void GenMCDriver::calcCoOrderings(WriteLabel *lab, const std::vector<EventLabel *> &cos)
{
	auto rp = cos.empty() ? nullptr : saveResumePoint(lab->getPos());
	for (const auto &predLab : cos) {
		auto fr = std::make_unique<WriteForwardRevisit>(lab->getPos(), predLab->getPos());
		if (rp) {
			fr->setResumePoint(rp);
			++rp->pending;
		}
		getExec().getWorkqueue().add(std::move(fr));
	}
}

auto GenMCDriver::saveResumePoint(Event pos) -> std::shared_ptr<ResumePoint>
{
	/* Only a verification run resumes: estimation and random runs do not
	 * revisit, an error replay collects its metadata from the start,
	 * relocated kept events (LKMM) move within a run, replay keys count
	 * from the start, and labels added from the instruction cache have no
	 * runtime state behind them */
	if (!resumeHook_ || getConf()->resumePoints == 0 || !inVerificationMode() ||
	    getScheduler().inErrorReplay() || relocatesKeptEvents() || checksReplayKeys() ||
	    getConf()->instructionCaching)
		return nullptr;
	/* The revisit restricts the graph to the event's stamp. A label of an
	 * event the runtime already passed may have been re-added since (e.g.,
	 * a spinloop's end reconsidered), with a later stamp: the restriction
	 * would remove it, leaving the saved state past the graph */
	const auto &g = getExec().getGraph();
	if (g.getMaxStamp() != g.getEventLabel(pos)->getStamp())
		return nullptr;
	auto rp = resumeHook_(pos);
	if (!rp)
		return nullptr;
	if (checksInvariants())
		rp->fingerprint = invariants::fingerprint(g, pos);
	resumePoints_.push_back(rp);
	if (resumePoints_.size() > getConf()->resumePoints)
		resumePoints_.pop_front();
	return rp;
}

auto GenMCDriver::handleStore(std::unique_ptr<WriteLabel> wLab, std::optional<SVal> oldVal)
	-> HandleResult<bool>
{
	auto &g = getExec().getGraph();

	/* The run was dropped while aligning this event (alignKeptEvents()) */
	if (droppedBeforeKept_)
		return {.result = Reset{}, .count = 0U};

	auto *lab = genmc::dyn_cast<WriteLabel>(addLabelToGraph(std::move(wLab)));

	/* Stores cannot cause atomicity violation:
	 * - In normal mode, non-maximal RMW are completed elsewhere
	 * - In estimation mode, we have already filtered violations on the read part */
	auto err = checkAccessValidity(lab->getPos(), lab->getAccess())
			   .or_else([&] { return checkInitializedMem(lab); })
			   .or_else([&] { return checkForMixedSize(lab); });
	if (err)
		return {.result = *err};

	/* Runtime sends the NA value lazily when adding next atomic access */
	if (oldVal)
		g.updateDeferredValue(lab->getAccess(), *oldVal);

	checkReconsiderFaiSpinloop(lab);
	unblockWaitingHelping(lab);
	checkReconsiderReadOpts(lab);

	/* Find all possible placings in coherence for this store. More than one
	 * means a write-write race where the placings respect hb; under LKMM
	 * they do not (the checker rejects the placings that break its axioms),
	 * and the kat's own warning rule decides (see checkForRaces()) */
	auto cos = getConsChecker().getCoherentPlacings(lab);
	/* -exhaustive under IMM: as for a read (see handleLoad()); the store
	 * still goes in coherence (last), for the graph to stay well formed
	 * until the run is dropped */
	if (cos.empty() && getConf()->exhaustive && getConf()->model == ModelType::IMM) {
		lab->addCo(g.co_max(lab->getAddr()));
		moot();
		return {.result = Invalid{}, .count = 1U};
	}
	const auto *racyLab =
		cos.size() > 1 && getConf()->model != ModelType::LKMM ? cos[0] : nullptr;

	const EventLabel *co = nullptr;
	if (inEstimationMode() || inRandomMode()) {
		co = pickRandomCo(lab, cos);
		if (inEstimationMode())
			getExec().getChoiceMap().update(lab, cos);
	} else {
		co = findConsistentCo(lab, cos);
		calcCoOrderings(lab, cos);
	}

	/* Print a WW-race warning if appropriate (if this moots, exploration
	 * will anyway be cut), once the store is in coherence: reporting may
	 * check the whole graph (see reportError()) */
	if (racyLab)
		reportWarningOnce(lab->getPos(), VerificationError::VE_WWRace, racyLab);

	GENMC_DEBUG(LOG(VerbosityLevel::Debug2, "--- Added store {}\n{}", lab->getPos(),
			getExec().getGraph()););

	if (getScheduler().inErrorReplay())
		return {.result = lab->wasAddedMax(), .count = 1U};

	calcRevisits(lab);
	if (!co || violatesAtomicity(lab) || !isPlacementConsistent(lab)) {
		moot();
		return {.result = Invalid{}, .count = 1U};
	}
	/* srcu_read_unlock() closes a critical section, which orders events
	 * already in the graph (srcu-rscs), as rcu_read_unlock() does. A grace
	 * period of its srcu_struct inside the section waits for its own
	 * reader: no execution is consistent, report it as RCU's */
	if (genmc::isa<SrcuUnlockWriteLabel>(lab)) {
		if (const auto *gp = lkmm::srcuOwnGracePeriod(g, lab)) {
			reportError(gp->getPos(),
				    {gp->getPos(), VerificationError::VE_InvalidRcu,
				     "synchronize_srcu() in a read-side critical section of its "
				     "srcu_struct!"});
			return {.result = VerificationError::VE_InvalidRcu};
		}
		if (!getConsChecker().isConsistent(lab)) {
			moot();
			return {.result = Invalid{}, .count = 1U};
		}
	}
	if (auto err = checkFinalAnnotations(lab).or_else([&] { return checkForRaces(lab); }); err)
		return {.result = *err};

	VERIFY(!lab->isNotAtomic() || exploresPlainAccesses());
	if (lab->isNotAtomic())
		g.getState().onNAStore(lab->getPos(), lab->getAccess(),
				       getConsChecker().getHbView(lab), std::nullopt);
	else
		g.getState().onATStore(lab->getPos(), lab->getAccess(),
				       getConsChecker().getHbView(lab), lab->isEffectful(),
				       lab->isAtLeastRelease(), getLockRole(lab));
	return {.result = lab->wasAddedMax(), .count = 1U};
}

auto GenMCDriver::handleMalloc(Event pos, ASize size, uint64_t alignment, StorageDuration sdur,
			       StorageType styp, AddressSpace spc, const NameInfo *info,
			       const std::string &name, const EventDeps &deps, bool zeroed,
			       bool threadPrivate) -> HandleResult<SVal>
{
	auto &g = getExec().getGraph();

	if (!mallocProbeLab_)
		mallocProbeLab_ = std::make_unique<MallocLabel>(pos, size.get(), alignment, sdur,
								styp, spc, info, name, deps);

	auto addr = g.getState().onAlloc(pos, size, alignment, sdur, styp, spc);
	configureProbe(mallocProbeLab_.get(), pos, addr, size, deps);
	mallocProbeLab_->setZeroed(zeroed);
	mallocProbeLab_->setThreadPrivate(threadPrivate);

	auto guard = g.addScoped(mallocProbeLab_);
	auto *lab = guard.get();
	checkAllocationInvariants("allocating a block");
	/* calloc(): the block is initialized (to zero) by the allocation itself,
	 * so no store events are needed before it may be read */
	if (zeroed && addr != SAddr())
		g.getState().onZeroedAlloc(pos, lab->getAccess(), getConsChecker().getHbView(lab));
	if constexpr (Config::emitNALabels)
		guard.commit();

	if (addr == SAddr()) {
		reportError(pos, {pos, VerificationError::VE_Allocation,
				  "Allocator is out of memory\n", nullptr});
		guard.commit();
		return {.result = VerificationError::VE_Allocation, .count = 1U};
	}
	return {.result = SVal(addr.get()), .count = Config::emitNALabels ? 1U : 0U};
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

auto GenMCDriver::handleRetire(Event pos, SAddr loc, const EventDeps &deps)
	-> HandleResult<std::monostate>
{
	auto &g = getExec().getGraph();

	auto invalid = getInvalidFreeReason(loc, StorageDuration::SD_Heap);
	auto size = invalid ? ASize(0) : g.getState().getAllocAccess(loc).size;
	if (!retireProbeLab_)
		retireProbeLab_ = std::make_unique<HpRetireLabel>(pos, loc, deps);

	configureProbe(retireProbeLab_.get(), pos, loc, size, deps);

	auto guard = g.addScoped(retireProbeLab_);
	if constexpr (Config::emitNALabels)
		guard.commit();

	if (invalid) {
		guard.commit();
		reportError(pos, {pos, VerificationError::VE_FreeNonMalloc, *invalid});
		return {.result = VerificationError::VE_FreeNonMalloc};
	}

	auto *lab = guard.get();
	if (auto err = checkForRaces(lab); err) {
		guard.commit();
		return {.result = *err};
	}
	g.getState().onFree(pos, loc, true);
	return {.result = std::monostate(), .count = Config::emitNALabels ? 1U : 0U};
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

auto GenMCDriver::handleFree(Event pos, SAddr loc, StorageDuration sdur, const EventDeps &deps)
	-> HandleResult<std::monostate>
{
	auto &g = getExec().getGraph();

	/* An invalid free is reported at its event, which has no block to span */
	auto invalid = getInvalidFreeReason(loc, sdur);
	auto size = invalid ? ASize(0) : g.getState().getAllocAccess(loc).size;
	if (!freeProbeLab_)
		freeProbeLab_ = std::make_unique<FreeLabel>(pos, loc, size, deps);

	configureProbe(freeProbeLab_.get(), pos, loc, size, deps);

	auto guard = g.addScoped(freeProbeLab_);
	if constexpr (Config::emitNALabels)
		guard.commit();

	if (invalid) {
		guard.commit();
		reportError(pos, {pos, VerificationError::VE_FreeNonMalloc, *invalid});
		return {.result = VerificationError::VE_FreeNonMalloc};
	}

	auto *lab = guard.get();
	if (auto err = checkForRaces(lab); err) {
		guard.commit();
		return {.result = *err};
	}
	g.getState().onFree(pos, loc, false);
	return {.result = std::monostate(), .count = Config::emitNALabels ? 1U : 0U};
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

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto GenMCDriver::getPreviousVisibleAccessLabel(
	const EventLabel *start) const // NOLINT(readability-function-cognitive-complexity)
	-> const MemAccessLabel *
{
	const auto &g = getExec().getGraph();
	std::vector<Event> finalReads;

	for (const auto &lab : g.po_preds(start)) {
		if (const auto *rLab = genmc::dyn_cast<ReadLabel>(&lab)) {
			if (rLab->isConfirming())
				continue;
			if (rLab->getRf()) {
				auto *wLab = genmc::dyn_cast<WriteLabel>(rLab->getRf());
				if (wLab && wLab->isLocal())
					continue;
				if (wLab && wLab->isFinal()) {
					finalReads.push_back(rLab->getPos());
					continue;
				}
				if (std::ranges::any_of(finalReads, [&](const Event &l) {
					    const auto *lLab =
						    genmc::dyn_cast<ReadLabel>(g.getEventLabel(l));
					    return lLab->getAddr() == rLab->getAddr() &&
						   lLab->getSize() == rLab->getSize();
				    }))
					continue;
			}
			return rLab;
		}
		if (const auto *wLab = genmc::dyn_cast<WriteLabel>(&lab))
			if (!wLab->isFinal() && !wLab->isLocal())
				return wLab;
	}
	return nullptr; /* none found */
}

void GenMCDriver::mootExecutionIfFullyBlocked(EventLabel *bLab)
{
	auto &g = getExec().getGraph();

	/* -exhaustive: the read before the block is revisitable whatever its
	 * flags say, so the other threads may still unblock it */
	if (getConf()->exhaustive)
		return;

	const auto *lab = getPreviousVisibleAccessLabel(bLab);
	/* This part tries to match original check when NA are not present
	 * (but both checks are likely not sound when Local/Final attributes are present)
	 */
	if constexpr (!Config::emitNALabels) {
		auto previousNA = g.getState().getPreviousNA(lab->getThread());
		if (previousNA.first != 0 && (!lab || previousNA.first > lab->getIndex())) {
			/* Previous NA is a write */
			if (!previousNA.second)
				return;
			/* Previous NA is a read: check whether it is revisitable */
			auto *mLab =
				g.getEventLabel(Event(bLab->getThread(), previousNA.first - 1));
			VERIFY(mLab);
			if (!mLab->isRevisitable())
				moot();
			return;
		}
	}
	if (const auto *rLab = genmc::dyn_cast_if_present<ReadLabel>(lab))
		if (!rLab->isRevisitable() || !rLab->wasAddedMax())
			moot();
}

auto GenMCDriver::handleBlock(std::unique_ptr<BlockLabel> lab) -> HandleResult<std::monostate>
{
	/* Call addLabelToGraph first to cache the label (not before kept
	 * events, where blockThread() puts the block after them) */
	if (!getExec().getGraph().hasEventsAfter(lab->getThread(), lab->getIndex()))
		addLabelToGraph(lab->clone());
	blockThreadTryMoot(std::move(lab));
	return {.result = std::monostate(), .count = 1U};
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

void GenMCDriver::initiateErrorReplay(const ErrorDetails &details)
{
	auto &g = getExec().getGraph();

	/* Initiate an exploration, and block the current one if it's a hard error */
	getExec().getWorkqueue().add(std::make_unique<ReplayForwardRevisit>(
		std::ranges::begin(g.rlabels())->getPos(), details));
	if (details.shouldHalt)
		moot();
}

void GenMCDriver::haltErrorReplay()
{
	getScheduler().setErrorReplayEvent(std::nullopt);
	dbgInfo_.clear();
	bufferedError_ = std::nullopt;

	/* The error-collecting execution finished; we have to drop it */
	moot();
}

void GenMCDriver::reportError(Event pos, const ErrorDetails &details)
{
	auto &g = getExec().getGraph();
	auto &scheduler = getScheduler();

	/* If anyone has already detected an error, no need to report another */
	if (isHalting())
		return;

	/* Not an execution: a thread stopped before events that others observe
	 * (see alignKeptEvents()), or an event went before kept ones and the
	 * graph has not been checked since. Under -exhaustive and IMM, the
	 * views of the events kept after holes lack what fills them, and the
	 * run may have chosen what they allow and the views computed anew do
	 * not (a read of a write the reader's thread has seen overwritten):
	 * the graph is checked with its views computed anew (an inconsistent
	 * graph stays so as events are added) */
	if (relocatesKeptEvents() && !scheduler.inErrorReplay()) {
		if (lkmm::hasThreadBlockedBeforeKept(g)) {
			++result.relocationsLost;
			moot();
			return;
		}
		if (getConf()->exhaustive && getConf()->model == ModelType::IMM) {
			if (!isExecutionConsistent(g)) {
				moot();
				return;
			}
		} else if (addedBeforeKept_) {
			addedBeforeKept_ = false;
			if (!getConsChecker().isConsistent(g)) {
				moot();
				return;
			}
		}
		if (!checkPendingKept())
			return;
	}

	/* If under estimation, ignore soft errors; they're gonna be reported later on
	 * anyway */
	if (inEstimationMode() && !details.shouldHalt)
		return;

	/* If this is an error replay (e.g., when one instruction maps to many events, or
	 * under IMM), do not get into an infinite loop... */
	if (scheduler.inErrorReplay() && !scheduler.isErrorReplayEvent(pos))
		return;

	/* Before printing an error, do an extra run to collect metadata;
	 * when pruning, the graph cannot be replayed, so report directly */
	if (!scheduler.inErrorReplay() && !getConf()->maxGraphSize.has_value()) {
		initiateErrorReplay(details);
		return;
	}

	/* Metadata run is over: if the error is an invalid access, change the RF of the
	 * offending event to BOTTOM, so that we do not try to get its value. */
	auto *errLab = details.pos ? g.getEventLabel(*details.pos) : nullptr;
	if (errLab && isInvalidAccessError(details.type) && genmc::isa<ReadLabel>(errLab))
		genmc::dyn_cast<ReadLabel>(errLab)->setRf(nullptr);
	/* An error at a block is replayed up to the event before it (see
	 * forwardRevisit()): the block was not re-executed */
	if (const auto *bLab = genmc::dyn_cast_if_present<BlockLabel>(errLab))
		inheritDbgInfo(bLab->getPos().prev(), bLab);

	/* Print basic error message (graph) */
	std::ostringstream out;
	out << std::format("{}: {}!\n", isHardError(details.type) ? "Error" : "Warning",
			   details.type);
	if (errLab)
		out << std::format("Event {} ", errLab->getPos());
	if (details.racyLab != nullptr)
		out << std::format("conflicts with event {} ", details.racyLab->getPos());
	out << "in graph:\n";
	printGraph(g, dbgInfo_, out);

	/* Print an error trace (if desired), and the specific error message */
	if (getConf()->printErrorTrace && errLab) {
		printTraceBefore(dbgInfo_, errLab, out);
		if (details.racyLab != nullptr)
			printTraceBefore(dbgInfo_, details.racyLab, out);
	}
	if (!details.msg.empty())
		out << details.msg;
	result.message += out.str();

	/* Dump the graph into a file (DOT format) */
	if (!getConf()->dotFile.empty())
		dotPrintToFile(getConf()->dotFile, errLab,
			       errLab ? getPrefixView(errLab).clone() : nullptr, details.racyLab,
			       details.racyLab ? getPrefixView(details.racyLab).clone() : nullptr,
			       &getConsChecker(), dbgInfo_, getConf()->dotPrintOnlyClientEvents);

	/* Stop the error-collecting execution */
	haltErrorReplay();

	/* If this was a hard error, stop altogether */
	if (details.shouldHalt)
		halt(details.type);
}

auto GenMCDriver::reportWarningOnce(Event pos, VerificationError wcode,
				    const EventLabel *racyLab /* = nullptr */) -> bool
{
	/* Helper function to determine whether the warning should be treated as an error */
	auto shouldUpgradeWarning = [&](auto &wcode) {
		if (wcode != VerificationError::VE_WWRace)
			return std::make_pair(false, ""s);
		if (!getConf()->symmetryReduction && !getConf()->ipr)
			return std::make_pair(false, ""s);

		auto &g = getExec().getGraph();
		auto *lab = g.getEventLabel(pos);
		auto isSymmetric = [&](auto tid) {
			return g.getFirstThreadLabel(tid)->getSymmPredTid() != -1;
		};
		auto srUpgrade = getConf()->symmetryReduction &&
				 std::ranges::any_of(g.thr_ids(), isSymmetric);
		auto iprUpgrade =
			getConf()->ipr && std::ranges::any_of(g.samelocs(lab), [&](auto &oLab) {
				auto *rLab = genmc::dyn_cast<ReadLabel>(&oLab);
				return rLab && rLab->getAnnot();
			});
		auto upgrade = srUpgrade || iprUpgrade;

		/* Without the symmetries GenMC inferred itself (rather than the user
		 * declared), this would only be a warning: verification may be
		 * repeated without SR */
		if (upgrade) {
			/* IPR is GenMC's own optimization: verification may also be
			 * repeated without it */
			getResult().iprError = iprUpgrade;
			getResult().inferredSymmetryError =
				!iprUpgrade && std::ranges::none_of(g.thr_ids(), [&](auto tid) {
					return isSymmetric(tid) &&
					       g.getFirstThreadLabel(tid)->getThreadInfo().symmId !=
						       -1;
				});
		}
		auto [cause, cli] =
			iprUpgrade ? std::make_pair("in-place revisiting (IPR)"s, "-disable-ipr"s)
				   : std::make_pair("symmetry reduction (SR)"s, "-disable-sr"s);
		auto msg = "Unordered writes do not constitute a bug per se, though they often "
			   "indicate faulty design.\n" +
			   (upgrade ? ("This warning is treated as an error due to " + cause +
				       ".\n"
				       "You can use " +
				       cli + " to disable these features."s)
				    : ""s);
		return std::make_pair(upgrade, msg);
	};

	/* If the warning has been seen before, only report it if it's an error */
	auto [upgradeWarning, msg] = shouldUpgradeWarning(wcode);
	auto &knownWarnings = getResult().warnings;
	if (upgradeWarning || !knownWarnings.contains(wcode)) {
		reportError(pos, {pos, wcode, msg, racyLab, upgradeWarning});
	}
	if (!knownWarnings.contains(wcode))
		knownWarnings.insert(wcode);
	if (wcode == VerificationError::VE_WWRace)
		getExec().getGraph().markWWRacy(getExec().getGraph().getWriteLabel(pos));
	return upgradeWarning;
}

auto GenMCDriver::checkBarrierWellFormedness(BIncFaiWriteLabel *sLab) -> bool
{
	if (getConf()->disableBAM)
		return true;

	/* Find the latest round completion (or init, if none exists) */
	auto &g = getExec().getGraph();
	auto lastIt = std::ranges::find_if(g.rco(sLab->getAddr()), [sLab](const auto &lab) {
		return &lab != sLab &&
		       ((genmc::isa<BIncFaiWriteLabel>(&lab) &&
			 isLastInBarrierRound(genmc::dyn_cast<BIncFaiWriteLabel>(&lab))) ||
			lab.getOrdering() == MemOrdering::Relaxed);
	});

	/* Check whether the last barrier completion is hb;po;po-before SLAB */
	auto valid = getConsChecker().getHbView(g.po_imm_pred(sLab)).contains(lastIt->getPos());
	if (!valid) {
		reportError(sLab->getPos(),
			    {sLab->getPos(), VerificationError::VE_BarrierWellFormedness,
			     "Execution not barrier-well-formed!\n"});
	}
	return valid;
}

auto GenMCDriver::tryOptimizeBarrierRevisits(BIncFaiWriteLabel *sLab,
					     std::vector<ReadLabel *> & /*loads*/) -> bool
{
	if (getConf()->disableBAM)
		return false;

	if (!checkBarrierWellFormedness(sLab) || !isLastInBarrierRound(sLab))
		return true;

	/* Find reads to revisit. `loads` is disregarded because it
	 * might not contain some valid revisits (e.g., discarded due to
	 * maximality-related optimizations) */
	auto &g = *sLab->getParent();
	auto *wLab = genmc::dyn_cast<ReadLabel>(g.po_imm_pred(sLab))->getRf();
	VERIFY(wLab);
	std::vector<ReadLabel *> toRevisit;

	while (genmc::isa<BIncFaiWriteLabel>(wLab) &&
	       !isLastInBarrierRound(genmc::dyn_cast<BIncFaiWriteLabel>(wLab))) {
		auto *nLab = g.po_imm_succ(wLab);
		if (nLab) {
			VERIFY(genmc::isa<BWaitReadLabel>(nLab));
			toRevisit.push_back(genmc::cast<ReadLabel>(nLab));
		}
		wLab = genmc::dyn_cast<ReadLabel>(g.po_imm_pred(wLab))->getRf();
	}

	/* Finally, revisit in place */
	for (auto *lab : toRevisit) {
		VERIFY(genmc::isa<BWaitReadLabel>(lab));
		revisitInPlace(*constructBackwardRevisit(lab, sLab));
	}
	return true;
}

void GenMCDriver::tryOptimizeIPRs(const WriteLabel *sLab, std::vector<ReadLabel *> &loads)
{
	if (!getConf()->ipr)
		return;

	std::vector<ReadLabel *> toIPR;
	std::erase_if(loads, [&](auto *rLab) {
		/* Treatment of blocked CASes is different */
		auto blocked =
			!genmc::isa<CasReadLabel>(rLab) && rLab->getAnnot() &&
			!rLab->valueMakesAssumeSucceed(rLab->getAccessValue(rLab->getAccess()));
		if (blocked)
			toIPR.push_back(rLab);
		return blocked;
	});

	for (auto *rLab : toIPR)
		revisitInPlace(*constructBackwardRevisit(rLab, sLab));

	/* We also have to filter out some regular revisits */
	const auto *confLab = findPendingRMW(sLab);
	if (!confLab)
		return;

	std::erase_if(loads, [&](auto *rLab) {
		auto *rfLab = rLab->getRf();
		return rLab->getAnnot() && // must be like that
		       rfLab->getStamp() > rLab->getStamp() &&
		       !getPrefixView(sLab).contains(rfLab->getPos());
	});
}

auto GenMCDriver::removeCASReadIfBlocks(const ReadLabel *rLab, const EventLabel *sLab) -> bool
{
	auto &g = getExec().getGraph();
	/* This only affects annotated CASes (-exhaustive adds the read and lets
	 * the thread block on its value, like any other read: a blocked read
	 * stays a read, which its revisits expect) */
	if (getConf()->exhaustive || !rLab->getAnnot() || !genmc::isa<CasReadLabel>(rLab) ||
	    (!getConf()->ipr && !genmc::isa<LockCasReadLabel>(rLab)))
		return false;
	/* Skip if bounding is enabled */
	if (getConf()->bound.has_value())
		return false;
	/* ... or if the read is to stay before kept events: the block goes
	 * after them (see blockThread()), and the read is re-run */
	if (g.hasEventsAfter(rLab->getThread(), rLab->getIndex()))
		return false;

	/* If the CAS blocks, block thread altogether */
	auto val = sLab->getAccessValue(rLab->getAccess());
	if (rLab->valueMakesAssumeSucceed(val))
		return false;

	blockThread(g, ReadOptBlockLabel::create(rLab->getPos(), rLab->getAddr()));
	return true;
}

void GenMCDriver::checkReconsiderReadOpts(const WriteLabel *sLab)
{
	auto &g = getExec().getGraph();
	for (auto i = 0; i < g.getNumThreads(); i++) {
		auto *bLab = genmc::dyn_cast_if_present<ReadOptBlockLabel>(g.getLastThreadLabel(i));
		if (!bLab || bLab->getAddr() != sLab->getAddr())
			continue;
		unblockThread(g, bLab->getPos());
	}
}

void GenMCDriver::optimizeUnconfirmedRevisits(const WriteLabel *sLab,
					      std::vector<ReadLabel *> &loads)
{
	if (!getConf()->confirmation)
		return;

	auto &g = getExec().getGraph();

	/* If there is already a write with the same value, report a possible ABA */
	auto valid = std::ranges::count_if(g.co(sLab->getAddr()), [&](auto &wLab) {
		return wLab.getPos() != sLab->getPos() && wLab.getVal() == sLab->getVal();
	});
	if (sLab->getAddr().isStatic() &&
	    g.getInitLabel()->getAccessValue(sLab->getAccess()) == sLab->getVal())
		++valid;
	WARN_ON_ONCE(valid > 0 && std::ranges::count_if(
					  loads, [](auto *lab) { return lab->isConfirming(); }),
		     "confirmation-aba-found",
		     "Possible ABA pattern! Consider running without -confirmation.");

	/* Do not bother with revisits that will be unconfirmed/lead to ABAs */
	std::erase_if(loads, [&](auto *lab) {
		if (!lab->isConfirming())
			return false;

		const EventLabel *scLab = nullptr;
		auto *pLab = findMatchingSpeculativeRead(lab, scLab);
		ERROR_ON(!pLab, "Confirming CAS annotation error! "
				"Does a speculative read precede the "
				"confirming operation?");

		return !scLab;
	});
}

auto GenMCDriver::tryOptimizeRevisits(WriteLabel *sLab, std::vector<ReadLabel *> &loads) -> bool
{

	/* BAM */
	if (!getConf()->disableBAM) {
		if (auto *faiLab = genmc::dyn_cast<BIncFaiWriteLabel>(sLab)) {
			if (tryOptimizeBarrierRevisits(faiLab, loads))
				return true;
		}
	}

	/* IPR */
	tryOptimizeIPRs(sLab, loads);

	/* Confirmation: Do not bother with revisits that will lead to unconfirmed reads */
	if (getConf()->confirmation)
		optimizeUnconfirmedRevisits(sLab, loads);
	return false;
}

void GenMCDriver::revisitInPlace(const BackwardRevisit &br)
{
	VERIFY(!getConf()->bound.has_value());

	auto &g = getExec().getGraph();
	auto *rLab = g.getReadLabel(br.getPos());
	auto *sLab = g.getWriteLabel(br.getRev());

	VERIFY(genmc::isa<ReadLabel>(rLab));
	if (g.po_imm_succ(rLab))
		g.removeLast(rLab->getThread());
	rLab->setRf(sLab);
	updateLabelViews(rLab);
	rLab->setAddedMax(true); /* explicitly set for atomicity violations */

	/* CASes shouldn't be handled via IPRs */
	VERIFY(!rLab->valueMakesRMWSucceed(rLab->getReturnValue()));

	GENMC_DEBUG(LOG(VerbosityLevel::Debug1, "--- In-place revisiting {} <-- {}\n{}",
			rLab->getPos(), sLab->getPos(), getExec().getGraph()););
}

static void updatePredsWithPrefixView(const ExecutionGraph &g, VectorClock &preds,
				      const VectorClock &pporf)
{
	/* In addition to taking (preds U pporf), make sure pporf includes rfis */
	preds.update(pporf);

	if (!dynamic_cast<const DepExecutionGraph *>(&g))
		return;
	auto &predsD = *genmc::dyn_cast<DepView>(&preds);
	for (auto i = 0; i < pporf.size(); i++) {
		for (auto j = 1; j <= pporf.getMax(i); j++) {
			const auto *lab = g.getEventLabel(Event(i, j));
			if (const auto *rLab = genmc::dyn_cast<ReadLabel>(lab)) {
				if (preds.contains(rLab->getPos()) &&
				    !preds.contains(rLab->getRf())) {
					if (rLab->getRf()->getThread() == rLab->getThread())
						predsD.removeHole(rLab->getRf()->getPos());
				}
			}
			const auto *wLab = genmc::dyn_cast<WriteLabel>(lab);
			if (wLab && wLab->isRMW() && pporf.contains(lab->getPos().prev()))
				predsD.removeHole(lab->getPos());
		}
	}
}

auto GenMCDriver::getRevisitView(const ReadLabel *rLab, const WriteLabel *sLab) const
	-> std::unique_ptr<VectorClock>
{
	const auto &g = getExec().getGraph();
	auto preds = g.getPredsView(rLab->getPos());

	/* A read in the write's prefix only through pctrl (-lkmm, see
	 * getRevisitPrefixView()): the prefix has events computed from the read's
	 * old value, so the revisit keeps the part of it without pctrl */
	const auto &prefix = getPrefixView(sLab);
	if (prefix.contains(rLab->getPos())) {
		updatePredsWithPrefixView(g, *preds, *getRevisitPrefixView(sLab));
		return preds;
	}
	updatePredsWithPrefixView(g, *preds, prefix);
	if (getConf()->exhaustive && getConf()->model == ModelType::IMM) {
		/* (a CAS that fails reading SLAB loses its write part) */
		if (failsReading(rLab, sLab) && preds->contains(rLab->getPos().next()))
			genmc::cast<DepView>(&*preds)->addHole(rLab->getPos().next());
		if (!keepValueDependencies(rLab->getPos(), *preds))
			return nullptr;
	}
	return preds;
}

/* -exhaustive under IMM. A revisit keeps the events added before the read and
 * the write's prefix, which is closed under (ppo | rfe)-predecessors; for a
 * kept read whose write is in its own thread, it also keeps the write (see
 * updatePredsWithPrefixView()), but not what the write depends on. That
 * write can be an RMW's write part whose read is not kept: its value is
 * then that of a read that is executed anew, and so is the value of the
 * kept read. The default exploration then updates the write when the read
 * is executed again (updateKeptRMWWrite()): it moves in coherence under the
 * readers it has, its revisits are queued with its old stamp (restricting
 * the graph to it cuts the read that completed it, and the revisiting
 * write), and its cached prefix view names events the graph no longer
 * has. Under -exhaustive, which takes revisits the maximality check
 * rejects, those cases end in crashes and in executions that are not
 * consistent or not executions of the program (issues #127, #135, #136).
 *
 * The reference keeps, instead, the graphs whose kept events are what the
 * program computes from the kept events: every event it keeps comes with
 * what its value and its existence depend on, its data, address, control
 * and CAS dependencies, an RMW's read for its write part, a read's write,
 * the creation of its thread, transitively. A revisit for which that
 * includes the revisited read, or an event the graph does not have, has no
 * such graph and is not taken. (A write is still kept without the events
 * it is merely ordered after: its value does not depend on them, and the
 * graph's consistency is checked once the revisit is done; see
 * backwardRevisit().) No kept write then changes when the threads fill
 * the holes, and none is completed anew */
auto GenMCDriver::keepValueDependencies(Event r, VectorClock &v) const -> bool
{
	const auto &g = getExec().getGraph();
	auto &dv = *genmc::cast<DepView>(&v);
	/* (Every view contains the begins: a begin is not kept for its own sake
	 * and keeps nothing, its thread's kept events keep the create) */
	std::vector<Event> work;
	for (const auto &lab : g.labels())
		if (lab.getPos() != r && !genmc::isa<EmptyLabel>(&lab) &&
		    !genmc::isa<ThreadStartLabel>(&lab) && v.contains(lab.getPos()))
			work.push_back(lab.getPos());
	while (!work.empty()) {
		const auto *lab = g.getEventLabel(work.back());
		work.pop_back();

		std::vector<Event> deps;
		const auto &d = lab->getDeps();
		for (const auto *di : {&d.data, &d.addr, &d.ctrl, &d.cas})
			std::ranges::copy(*di, std::back_inserter(deps));
		if (const auto *wLab = genmc::dyn_cast<WriteLabel>(lab); wLab && wLab->isRMW())
			deps.push_back(lab->getPos().prev());
		if (const auto *rLab = genmc::dyn_cast<ReadLabel>(lab); rLab && rLab->getRf())
			deps.push_back(rLab->getRf()->getPos());
		if (const auto *bLab = g.getFirstThreadLabel(lab->getThread());
		    bLab && bLab->getCreate())
			deps.push_back(bLab->getCreate()->getPos());

		for (auto e : deps) {
			if (e == r)
				return false;
			if (e.isInitializer() || v.contains(e))
				continue;
			const auto *eLab = g.getEventLabelIfPresent(e);
			if (!eLab || genmc::isa<EmptyLabel>(eLab))
				return false;
			dv.updateIdx(e);
			work.push_back(e);
		}
	}
	return true;
}

auto GenMCDriver::recomputeViews(ExecutionGraph &g) -> bool
{
	/* A label's views come from those of the labels before it in hb, which
	 * need not come first in the insertion order: iterate until the hb
	 * views settle (they only grow after the first pass; an hb cycle makes
	 * them grow until the bound). A hole takes its po-predecessor's views,
	 * which the events kept after it inherit */
	const auto hb = getConsChecker().getHbViewIndex();
	auto same = [&g](const View &a, const View &b) {
		return std::ranges::all_of(std::views::iota(0, g.getNumThreads()),
					   [&](auto t) { return a.getMax(t) == b.getMax(t); });
	};
	auto settled = false;
	for (auto pass = 0U; !settled && pass <= g.getMaxStamp().get() + 1; pass++) {
		settled = true;
		for (auto &lab : g.labels()) {
			if (genmc::isa<InitLabel>(&lab))
				continue;
			const auto had = lab.views().size() > hb;
			const auto old = had ? lab.view(hb) : View();
			updateLabelViews(&lab);
			settled &= had && same(old, lab.view(hb));
		}
	}
	for (auto &lab : g.labels())
		if (!genmc::isa<InitLabel>(&lab))
			lab.setPrefixView(nullptr);
	return settled;
}

auto GenMCDriver::isGraphConsistent(const ExecutionGraph &g) const -> bool
{
	const auto &cc = getConsChecker();
	auto coIndex = [&g](const EventLabel *wLab) -> int {
		if (genmc::isa<InitLabel>(wLab))
			return -1;
		auto i = 0;
		for (const auto &oLab : g.co(genmc::cast<WriteLabel>(wLab)->getAddr())) {
			if (&oLab == wLab)
				return i;
			++i;
		}
		VERIFY(false);
		return -1;
	};

	/* Coherence: no cycle in hb;eco, as CoWR, CoRW, CoRR and CoWW */
	for (const auto &lab : g.labels()) {
		const auto *rLab = genmc::dyn_cast<ReadLabel>(&lab);
		if (!rLab || !rLab->getRf())
			continue;
		const auto &hbR = cc.getHbView(rLab);
		const auto rf = coIndex(rLab->getRf());
		auto i = 0;
		for (const auto &wLab : g.co(rLab->getAddr())) {
			if (i > rf && hbR.contains(wLab.getPos()))
				return false;
			if (i <= rf && cc.getHbView(&wLab).contains(rLab->getPos()))
				return false;
			++i;
		}
		for (const auto &oLab : g.labels()) {
			const auto *oRLab = genmc::dyn_cast<ReadLabel>(&oLab);
			if (oRLab && oRLab != rLab && oRLab->getRf() &&
			    oRLab->getAddr() == rLab->getAddr() && hbR.contains(oRLab->getPos()) &&
			    coIndex(oRLab->getRf()) > rf)
				return false;
		}
	}
	for (auto locIt = g.loc_begin(); locIt != g.loc_end(); ++locIt) {
		auto i = 0;
		for (const auto &wLab : g.co(locIt->first)) {
			auto j = 0;
			for (const auto &oLab : g.co(locIt->first)) {
				if (j++ >= i)
					break;
				if (cc.getHbView(&oLab).contains(wLab.getPos()))
					return false;
			}
			++i;
		}
	}

	/* Atomicity: an RMW's write follows the write its read reads from */
	for (const auto &lab : g.labels()) {
		const auto *wLab = genmc::dyn_cast<WriteLabel>(&lab);
		if (!wLab || !wLab->isRMW())
			continue;
		const auto *rLab = genmc::dyn_cast<ReadLabel>(g.po_imm_pred(wLab));
		const EventLabel *pLab = g.co_imm_pred(wLab);
		if (!rLab || rLab->getRf() != (pLab ? pLab : g.getInitLabel()))
			return false;
	}
	return cc.isConsistent(g);
}

auto GenMCDriver::isExecutionConsistent(const ExecutionGraph &g) -> bool
{
	auto copy = g.clone();
	getConsChecker().recomputeCacheCounters(*copy);
	const auto consistent = recomputeViews(*copy) && isGraphConsistent(*copy);
	getConsChecker().recomputeCacheCounters(g);
	return consistent;
}

/* The errors and warnings found at an event (checkForRaces()) are pairs
 * the views leave unordered: a race, unordered writes, an access to freed
 * memory. Under -exhaustive and IMM, the events a revisit kept after a
 * hole keep the views they had when they were kept (see
 * isExecutionConsistent()), and so do the events added after them: what
 * the event that fills the hole orders before them is missing (e.g., the
 * write a kept write's thread now reads with acquire before it). Such a
 * pair can be ordered once the views are computed anew, and the views
 * computed anew only grow: what they leave unordered is all that is.
 * (Whether the graph is consistent is checked before an error is
 * reported, see reportError().) */
void GenMCDriver::checkWithViewsAnew(const std::function<void(const ExecutionGraph &)> &check)
{
	const auto &g = getExec().getGraph();
	auto copy = g.clone();
	getConsChecker().recomputeCacheCounters(*copy);
	recomputeViews(*copy);
	check(*copy);
	getConsChecker().recomputeCacheCounters(g);
}

auto GenMCDriver::constructBackwardRevisit(const ReadLabel *rLab, const WriteLabel *sLab) const
	-> std::unique_ptr<BackwardRevisit>
{
	return std::make_unique<BackwardRevisit>(rLab, sLab, getRevisitView(rLab, sLab));
}

static auto isReadInView(const WriteLabel *sLab, const DepView &v) -> bool
{
	return std::ranges::any_of(sLab->readers(),
				   [&v](auto &rLab) { return v.contains(rLab.getPos()); });
}

auto GenMCDriver::isCoBeforeSavedPrefix(const BackwardRevisit &r, const EventLabel *lab) -> bool
{
	const auto *mLab = genmc::dyn_cast<MemAccessLabel>(lab);
	if (!mLab)
		return false;

	auto &g = getExec().getGraph();
	const auto &v = r.getViewNoRel();
	const auto *rLab = genmc::dyn_cast<ReadLabel>(mLab);
	auto *wLab = g.getWriteLabel(rLab ? rLab->getRf()->getPos() : mLab->getPos());

	auto succs = wLab ? g.co_succs(wLab) : g.co(mLab->getAddr());
	return std::ranges::any_of(succs, [&](auto &sLab) {
		/* Exclude the write that revisits from the prefix */
		return sLab.getPos() != r.getRev() && v->contains(sLab.getPos());
	});
}

/* Dependency-tracking models: R deletes LAB, or LAB is the read R revisits.
 * Whether LAB reads from (a read) or follows in coherence (a write) the
 * co-latest write it can reach among the writes R keeps, except the revisiting
 * one, and those added before LAB: TruSt's condition, that LAB made the
 * maximal choice among the writes present when it was added and that no kept
 * write could have revisited it (as isCoBeforeSavedPrefix() requires for the
 * other models), with the kept writes present from the start. If LAB chose
 * otherwise, the graph where it chose that write is the one R is explored
 * from.
 *
 * R keeps the revisiting write's prefix, which is not po-closed: it may keep
 * accesses of LAB's thread po-after LAB (LAB is then executed anew into a
 * hole), and a kept event was not always added after the deleted ones.
 * Coherence bounds LAB by the first po-later access of its thread to its
 * location present when LAB is added (kept, or added before LAB): LAB
 * precedes a write, and precedes or reads the write a read reads, as
 * coMaxReachable() bounds an access executed into a hole. The writes past
 * the bound are no alternatives. The added-maximally flag judges LAB against
 * the graph of the time it was added, which may lack a kept access that
 * bounds it (#121); and a kept write co-after LAB's is an alternative even
 * when its prefix reaches LAB's thread past LAB through a hole (#128) */
auto GenMCDriver::isPlacedMaximallyInRevisit(const BackwardRevisit &r,
					     const MemAccessLabel *lab) const -> bool
{
	const auto &g = getExec().getGraph();
	const auto &v = r.getViewNoRel();

	/* A weak CAS that failed spuriously is the non-maximal twin of its success */
	if (const auto *cLab = genmc::dyn_cast<CasReadLabel>(lab); cLab && cLab->failsSpuriously())
		return false;

	/* An RMW's write part kept while its read is deleted follows the write
	 * that read gets when it is executed anew (updateKeptRMWWrite()): for a
	 * write LAB, it is placed when its read is added, not before */
	auto floats = [&](const EventLabel &eLab) {
		const auto *sLab = genmc::dyn_cast<WriteLabel>(&eLab);
		return genmc::isa<WriteLabel>(lab) && sLab && sLab->isRMW() &&
		       v->contains(eLab.getPos()) && !v->contains(eLab.getPos().prev());
	};
	auto isKept = [&](const EventLabel &eLab) {
		return v->contains(eLab.getPos()) && !floats(eLab);
	};
	auto addedAt = [&](const EventLabel &eLab) {
		return floats(eLab) ? g.getEventLabel(eLab.getPos().prev())->getStamp()
				    : eLab.getStamp();
	};
	auto competes = [&](const WriteLabel &sLab) {
		return sLab.getPos() != r.getRev() &&
		       (isKept(sLab) || addedAt(sLab) < lab->getStamp());
	};

	/* An RMW read's own write goes right after the read's write, wherever
	 * that is: it is no bound and no alternative, nor are the reads of it */
	const auto *rLab = genmc::dyn_cast<ReadLabel>(lab);
	const auto *ownLab = rLab && rLab->isRMW() ? g.getWriteLabel(lab->getPos().next())
						   : nullptr;

	/* The bound: LAB precedes BOUND, or may read it (BOUND a read's write) */
	const EventLabel *bound = nullptr;
	auto mayReadBound = false;
	for (auto j = lab->getIndex() + 1; j < g.getThreadSize(lab->getThread()); j++) {
		const auto *mLab = genmc::dyn_cast<MemAccessLabel>(
			g.getEventLabel(Event(lab->getThread(), j)));
		if (!mLab || mLab == ownLab || mLab->getAddr() != lab->getAddr())
			continue;
		/* An access deleted and added after LAB is re-added after it */
		if (!isKept(*mLab) && addedAt(*mLab) > lab->getStamp())
			continue;
		if (const auto *qLab = genmc::dyn_cast<ReadLabel>(mLab)) {
			if (!qLab->getRf() || qLab->getRf() == ownLab)
				continue;
			bound = qLab->getRf();
			mayReadBound = true;
		} else {
			bound = mLab;
		}
		break;
	}

	const auto *wLab = rLab ? rLab->getRf() : lab;
	VERIFY(wLab);

	/* A read reads a write added after it only through a revisit by that
	 * write: deleted, that write is no alternative, and the graph before
	 * its revisit builds the same graph (TruSt's revisitable flag) */
	const auto *rfLab = genmc::dyn_cast<WriteLabel>(wLab);
	if (rLab && rfLab && !competes(*rfLab))
		return false;
	if (bound && wLab == bound)
		return true;

	auto succs = rfLab ? g.co_succs(rfLab) : g.co(lab->getAddr());
	for (const auto &sLab : succs) {
		if (bound && &sLab == bound)
			return !(rLab && mayReadBound && competes(sLab));
		if (&sLab != ownLab && competes(sLab))
			return false;
	}
	return true;
}

/* Whether LAB, a write to the location of the revisiting write, follows in
 * coherence a write that the revisit keeps although that write was added after
 * LAB and is no RMW (it was placed non-maximally). Under LKMM, whose prefix keeps
 * a write po-before the revisiting one to the same location (moi), the revisit
 * deletes LAB and hides that placement; the same graph then follows from the
 * revisit of the kept write itself (tests/correct/lkmm/dup-revisit-nested-if).
 * Only when that other path exists: the read is no RMW and could read from the
 * kept write (which follows the read's write in coherence). Otherwise, the
 * graph where the kept write is placed maximally is rejected too (the read's
 * write precedes the kept one: isPlacedMaximallyInRevisit(), or
 * isCoBeforeSavedPrefix() for the other models), and this one is the only
 * path */
auto GenMCDriver::isCoAfterLaterSavedWrite(const BackwardRevisit &r, const EventLabel *lab) -> bool
{
	const auto *wLab = genmc::dyn_cast<WriteLabel>(lab);
	if (!wLab)
		return false;

	auto &g = getExec().getGraph();
	const auto *rLab = g.getReadLabel(r.getPos());
	if (rLab->isRMW() || wLab->getAddr() != g.getWriteLabel(r.getRev())->getAddr())
		return false;

	/* The kept write could have been read instead of the read's write */
	const auto *rfLab = genmc::dyn_cast<WriteLabel>(rLab->getRf());
	auto readable = [&](auto &pLab) {
		return !rfLab || rfLab == &pLab ||
		       std::ranges::any_of(g.co_succs(rfLab),
					   [&](auto &sLab) { return &sLab == &pLab; });
	};
	const auto &v = r.getViewNoRel();
	return std::ranges::any_of(g.co_preds(wLab), [&](auto &pLab) {
		return pLab.getPos() != r.getRev() && !pLab.isRMW() && !pLab.wasAddedMax() &&
		       v->contains(pLab.getPos()) && pLab.getStamp() > wLab->getStamp() &&
		       readable(pLab);
	});
}

auto GenMCDriver::coherenceSuccRemainInGraph(const BackwardRevisit &r) -> bool
{
	auto &g = getExec().getGraph();
	auto *wLab = g.getWriteLabel(r.getRev());
	if (wLab->isRMW())
		return true;

	auto succs = g.co_succs(wLab);
	if (std::ranges::empty(succs))
		return true;

	return r.getViewNoRel()->contains(&*std::ranges::begin(succs));
}

static auto wasAddedMaximally(const EventLabel *lab) -> bool
{
	if (const auto *mLab = genmc::dyn_cast<MemAccessLabel>(lab))
		return mLab->wasAddedMax();
	if (const auto *oLab = genmc::dyn_cast<OptionalLabel>(lab))
		return !oLab->isExpanded();
	return true;
}

/* Whether LAB, not revisitable, is in the prefix of a write that R keeps and
 * that revisited a read still reading it (the read's stamp is below the
 * write's) */
auto GenMCDriver::isKeptByKeptRevisit(const BackwardRevisit &r, const EventLabel *lab) const -> bool
{
	const auto &g = getExec().getGraph();
	const auto &v = r.getViewNoRel();
	return std::ranges::any_of(g.labels(), [&](auto &oLab) {
		const auto *uLab = genmc::dyn_cast<WriteLabel>(&oLab);
		if (!uLab || uLab->getPos() == r.getRev() || !v->contains(uLab->getPos()))
			return false;
		auto revisited = std::ranges::any_of(uLab->readers(), [&](auto &qLab) {
			return qLab.getStamp() < uLab->getStamp();
		});
		return revisited && getRevisitPrefixView(uLab)->contains(lab->getPos());
	});
}

auto GenMCDriver::isMaximalExtension(const BackwardRevisit &r) -> bool
{
	/* Only revisit when the write's direct successor (if any) remains in the graph;
	 * revisits should not only differ on the write's placement */
	if (!coherenceSuccRemainInGraph(r))
		return false;

	auto &g = getExec().getGraph();
	const auto &v = r.getViewNoRel();

	const auto *depV =
		getConf()->isDepTrackingModel
			? genmc::dyn_cast<DepView>(&getPrefixView(g.getEventLabel(r.getRev())))
			: nullptr;

	/* Dependency-tracking models judge the accesses the revisit deletes
	 * against the writes they can reach (isPlacedMaximallyInRevisit()), not
	 * by their flags. A deleted event that an earlier revisit kept (not
	 * revisitable) still rejects the revisit, since the graph before that
	 * revisit leads to the same one, unless this revisit keeps the earlier
	 * revisiting write: that write is then an alternative for the read it
	 * revisited, and the other graph is the one rejected */
	for (const auto &lab : g.labels()) {
		/* Exclude events unaffected by the revisit */
		if (lab.getPos() != r.getPos() && v->contains(lab.getPos()))
			continue;

		if (depV) {
			if (const auto *sLab = genmc::dyn_cast<WriteLabel>(&lab);
			    sLab && isReadInView(sLab, *depV))
				continue;
			if (!lab.isRevisitable() && !isKeptByKeptRevisit(r, &lab))
				return false;
			/* (Under BAM, a barrier wait reads its round's last increment,
			 * revisited in place: no choice of its own to judge) */
			const auto *mLab = genmc::dyn_cast<MemAccessLabel>(&lab);
			if (mLab && (getConf()->disableBAM || !genmc::isa<BWaitReadLabel>(mLab))) {
				if (!isPlacedMaximallyInRevisit(r, mLab))
					return false;
			} else if (!wasAddedMaximally(&lab)) {
				return false;
			}
			if (isCoAfterLaterSavedWrite(r, &lab))
				return false;
			continue;
		}

		if (!lab.isRevisitable())
			return false;
		if (!wasAddedMaximally(&lab))
			return false;
		if (isCoBeforeSavedPrefix(r, &lab))
			return false;
		if (isCoAfterLaterSavedWrite(r, &lab))
			return false;
	}
	return true;
}

auto GenMCDriver::copyGraph(const BackwardRevisit *br, VectorClock *v) const
	-> std::unique_ptr<ExecutionGraph>
{
	const auto &g = getExec().getGraph();

	/* Adjust the view that will be used for copying */
	auto prefix = getRevisitPrefixView(g.getEventLabel(br->getRev()));
	if (checksInvariants())
		invariants::checkCopyView(g, *v,
					  invariantContext("copying the graph for a backward revisit"));
	auto plan = checksInvariants() ? std::optional(invariants::planCopyUpTo(g, *v))
				       : std::nullopt;
	auto og = g.getCopyUpTo(*v);
	checkGraphInvariants(*og, "copying the graph for a backward revisit",
			     plan ? &*plan : nullptr);

	/** Ensure the prefix of the write will not be revisitable.
	 * This is also used to check whether a write has revisited,
	 * and appropriately prevent some revisits
	 * (-exhaustive: every read stays revisitable, so that the exploration
	 * from a graph does not depend on the path that reached it) */
	if (getConf()->exhaustive)
		return og;
	for (auto &lab : og->labels()) {
		if (prefix->contains(lab.getPos()))
			lab.setRevisitStatus(false);
	}
	return og;
}

void GenMCDriver::calcRevisits(WriteLabel *sLab)
{
	if (!checkBlindRMWWellFormedness(sLab))
		return;

	/* Random mode neither revisits nor IPRs */
	if (inRandomMode())
		return;

	auto loads = getRevisitableApproximation(sLab);

	GENMC_DEBUG(LOG(VerbosityLevel::Debug3, "Revisitable: {}", loads););
	if (tryOptimizeRevisits(sLab, loads))
		return;

	/* If operating in estimation mode, don't actually revisit */
	if (inEstimationMode()) {
		getExec().getChoiceMap().update(loads, sLab);
		return;
	}

	GENMC_DEBUG(LOG(VerbosityLevel::Debug3, "Revisitable (optimized): {}", loads););
	for (auto *rLab : loads) {
		auto br = constructBackwardRevisit(rLab, sLab);
		/* -exhaustive: every revisit whose graph is well formed and can be
		 * consistent (the visited set, not maximality, bounds the exploration) */
		if (getConf()->exhaustive) {
			if (br->getViewNoRel() && isRevisitViewClosed(*br) && !keepsConflictingRMW(*br))
				getExec().getWorkqueue().add(std::move(br));
			continue;
		}
		if (!isMaximalExtension(*br))
			break;

		getExec().getWorkqueue().add(std::move(br));
	}
}

/* The revisit keeps the events added before the read and the write's prefix.
 * A kept read may read from a write the revisit deletes: one added after the
 * read that revisited it earlier (the read's thread may have kept events in
 * between, through that write's prefix). TruSt rejects such a revisit by the
 * revisitable flag it set on that write's prefix; -exhaustive ignores the
 * flags, so it checks the graph: every kept read keeps its write */
auto GenMCDriver::isRevisitViewClosed(const BackwardRevisit &r) -> bool
{
	auto &g = getExec().getGraph();
	const auto &v = r.getViewNoRel();
	return std::ranges::all_of(g.labels(), [&](auto &lab) {
		const auto *rLab = genmc::dyn_cast<ReadLabel>(&lab);
		if (!rLab || rLab->getPos() == r.getPos() || !v->contains(rLab->getPos()) ||
		    !rLab->getRf())
			return true;
		return v->contains(rLab->getRf()->getPos());
	});
}

/* The revisiting RMW's read keeps its write W; another RMW reading from W in
 * the revisit's view would make both RMWs read W, a graph no extension
 * repairs (and backwardRevisit() requires its absence) */
auto GenMCDriver::keepsConflictingRMW(const BackwardRevisit &r) -> bool
{
	auto &g = getExec().getGraph();
	const auto *sLab = g.getWriteLabel(r.getRev());
	if (!sLab->isRMW())
		return false;
	const auto *pLab = genmc::dyn_cast<ReadLabel>(g.po_imm_pred(sLab));
	VERIFY(pLab && pLab->getRf());
	/* (The revisited read is in its own view, but it will read from SLAB) */
	const auto &v = r.getViewNoRel();
	auto conflicts = [&](const ReadLabel &rLab) {
		return &rLab != pLab && rLab.getPos() != r.getPos() && rLab.isRMW() &&
		       v->contains(rLab.getPos());
	};
	if (const auto *wLab = genmc::dyn_cast<WriteLabel>(pLab->getRf()))
		return std::ranges::any_of(wLab->readers(), conflicts);
	return std::ranges::any_of(g.getInitLabel()->rfs(pLab->getAddr()), conflicts);
}

auto GenMCDriver::completeRevisitedRMW(const ReadLabel *rLab) -> WriteLabel *
{
	auto wLab = createRMWWriteLabel(getExec().getGraph(), rLab);
	if (!wLab)
		return nullptr;

	/* The read's own write part may have been kept while the read was not
	 * (a dependency-tracking model's revisit keeps the write its prefix
	 * depends on, and the read is re-executed): the write stays */
	if (rLab->isRMW())
		return updateKeptRMWWrite(rLab);

	/* The read was followed by kept events (a failed CAS has no write, and
	 * the events after it may not depend on it): make room for the write */
	if (const auto *nLab = getExec().getGraph().getEventLabelIfPresent(wLab->getPos());
	    nLab && !genmc::isa<EmptyLabel>(nLab))
		relocateExecution(wLab->getThread(), wLab->getIndex(), 1);

	/* Added for the read's instruction, as its next event */
	if (rLab->getKey())
		wLab->setKey(rLab->getSharedKey(), rLab->getKeySub() + 1);
	auto *lab = genmc::dyn_cast<WriteLabel>(addLabelToGraph(std::move(wLab)));
	VERIFY(rLab->getRf());
	lab->addCo(rLab->getRf());
	return lab;
}

/* RLAB's write part, kept through a revisit while the read was not, takes
 * the value and the coherence position the read's rf now gives it, and its
 * views and revisits are computed anew, as for a write just added. (A CAS
 * that now fails keeps its write: the write stays as kept.) */
auto GenMCDriver::updateKeptRMWWrite(const ReadLabel *rLab) -> WriteLabel *
{
	auto &g = getExec().getGraph();
	auto *kLab = g.getWriteLabel(rLab->getPos().next());
	VERIFY(kLab && kLab->isRMW() && rLab->getRf());
	auto wLab = createRMWWriteLabel(g, rLab);
	if (!wLab)
		return nullptr;
	kLab->setVal(wLab->getVal());
	kLab->moveCo(rLab->getRf());
	updateLabelViews(kLab);
	return kLab;
}

auto GenMCDriver::revisitWrite(const WriteForwardRevisit &ri) -> bool
{
	auto &g = getExec().getGraph();
	auto *wLab = g.getWriteLabel(ri.getPos());
	VERIFY(wLab);

	wLab->moveCo(g.getEventLabel(ri.getPred()));
	calcRevisits(wLab);
	return !violatesAtomicity(wLab);
}

auto GenMCDriver::revisitOptional(const OptionalForwardRevisit &oi) -> bool
{
	auto &g = getExec().getGraph();
	auto *oLab = genmc::dyn_cast<OptionalLabel>(g.getEventLabel(oi.getPos()));

	VERIFY(oLab);
	oLab->setExpandable(false);
	oLab->setExpanded(true);
	return true;
}

auto GenMCDriver::revisitRead(const Revisit &ri) -> bool
{
	VERIFY(genmc::isa<ReadRevisit>(&ri));

	/* We are dealing with a read: change its reads-from and also check
	 * whether a part of an RMW should be added */
	auto &g = getExec().getGraph();
	auto *rLab = g.getReadLabel(ri.getPos());
	auto *revLab = g.getEventLabel(genmc::dyn_cast<ReadRevisit>(&ri)->getRev());

	rLab->setRf(revLab);
	updateLabelViews(rLab);
	/* A revisited weak CAS is a fresh instance of the read: whether it
	 * fails spuriously is decided again (see maybePushWeakCasFailure()).
	 * The flag of the failing instance explored before it would otherwise
	 * turn the new one into a failure too, whatever it reads */
	if (auto *cLab = genmc::dyn_cast<CasReadLabel>(rLab))
		cLab->setSpuriousFailure(false);
	const auto *fri = genmc::dyn_cast<ReadForwardRevisit>(&ri);

	GENMC_DEBUG(LOG(VerbosityLevel::Debug1, "--- {} revisiting {} <-- {}\n{}",
			(genmc::isa<BackwardRevisit>(&ri) ? "Backward" : "Forward"), ri.getPos(),
			revLab->getPos(), getExec().getGraph()););

	/* If the revisited read is a blocking CAS, try to remove it.
	 * We have to be careful not to do this in the "forward" path (when the CAS is
	 * re-scheduled): in that case, the CAS does need to block. This path doesn't typically
	 * go through revisitRead(). However, if we examine rfs in a non-standard order,
	 * it might (hence the first clause below). */
	if ((!fri || !fri->isMaximal()) && removeCASReadIfBlocks(rLab, revLab))
		return true;

	/* If the revisited label became an RMW, add the store part and revisit */
	if (auto *sLab = completeRevisitedRMW(rLab)) {
		/* A weak CAS that just succeeded also needs its failing execution */
		if (auto *cLab = genmc::dyn_cast<CasReadLabel>(rLab))
			maybePushWeakCasFailure(cLab);
		calcRevisits(sLab);
		return !violatesAtomicity(sLab);
	}

	/* Blocked barrier or blocked lock: block thread */
	if (genmc::isa<BWaitReadLabel>(rLab) &&
	    !readsBarrierUnblockingValue(genmc::cast<BWaitReadLabel>(rLab)))
		blockThread(g, BarrierBlockLabel::create(rLab->getPos().next()));
	return true;
}

auto GenMCDriver::forwardRevisit(const ForwardRevisit &fr) -> bool
{
	auto &g = getExec().getGraph();
	if (const auto *mi = genmc::dyn_cast<WriteForwardRevisit>(&fr))
		return revisitWrite(*mi);
	if (const auto *oi = genmc::dyn_cast<OptionalForwardRevisit>(&fr))
		return revisitOptional(*oi);
	if (genmc::isa<RerunForwardRevisit>(&fr))
		return true;
	if (const auto *rr = genmc::dyn_cast<ReplayForwardRevisit>(&fr)) {
		/* The replay runs up to the event that added the last one: a block
		 * is added after its thread's last event, a thread's begin by its
		 * create */
		auto *errLab = g.getEventLabel(fr.getPos());
		const auto *bLab = genmc::dyn_cast<ThreadStartLabel>(errLab);
		getScheduler().setErrorReplayEvent(
			genmc::isa<BlockLabel>(errLab) ? errLab->getPos().prev()
			: bLab && bLab->getCreate()    ? bLab->getCreate()->getPos()
						       : errLab->getPos());
		bufferedError_ = rr->getDetails();
		return true;
	}
	if (const auto *wcf = genmc::dyn_cast<WeakCasFailureRevisit>(&fr))
		return revisitWeakCasFailure(*wcf);
	const auto *ri = genmc::dyn_cast<ReadForwardRevisit>(&fr);
	VERIFY(ri);
	return revisitRead(*ri);
}

auto GenMCDriver::revisitWeakCasFailure(const WeakCasFailureRevisit &ri) -> bool
{
	auto &g = getExec().getGraph();
	auto *rLab = g.getReadLabel(ri.getPos());
	auto *cLab = genmc::cast<CasReadLabel>(rLab);

	rLab->setRf(g.getEventLabel(ri.getRev()));
	updateLabelViews(rLab);
	cLab->setSpuriousFailure(true);
	rLab->setAddedMax(false); /* spurious failure is non-maximal */
	return true;
}

auto GenMCDriver::backwardRevisit(const BackwardRevisit &br) -> bool
{
	auto &g = getExec().getGraph();

	/* Recalculate the view because some B labels might have been
	 * removed */
	auto v = getRevisitView(g.getReadLabel(br.getPos()), g.getWriteLabel(br.getRev()));
	if (!v) {
		VERIFY(getConf()->exhaustive);
		return false;
	}

	auto og = copyGraph(&br, &*v);
	auto cmap = ChoiceMap(getExec().getChoiceMap());
	cmap.cut(*v);

	pushExecution({std::move(og), LocalQueueT(), std::move(cmap)});

	repairDanglingReads(getExec().getGraph());

	/* -exhaustive under IMM: the copy keeps the views its labels had in
	 * the graph of the revisit, computed before the read read from the
	 * revisiting write. The events kept after the read (and after the holes
	 * of other threads) are ordered differently in the new graph; their
	 * views are computed anew, before the read's revisit and after it, and
	 * the graph is checked as a whole: coherence and atomicity, which the
	 * exploration otherwise ensures as it adds each event, do not cover the
	 * kept events, and the revisit is not taken if the graph is not
	 * consistent. (A revisit that pushed work of its own: it is dropped, so
	 * that the execution is popped) */
	const auto checksGraph = getConf()->exhaustive && getConf()->model == ModelType::IMM;
	auto succeeded = !checksGraph || recomputeViews(getExec().getGraph());
	succeeded = succeeded && revisitRead(br);
	if (checksGraph && (!succeeded || !recomputeViews(getExec().getGraph()) ||
			    !isGraphConsistent(getExec().getGraph()))) {
		getExec().getWorkqueue().truncate(0);
		return false;
	}
	VERIFY(succeeded);

	/* If there are idle workers in the thread pool,
	 * try submitting the job instead */
	static constexpr auto kTaskFillFactor = 8; /* max tasks per worker before blocking */
	auto *pool = getThreadPool();
	if (pool && pool->getRemainingTasks() < kTaskFillFactor * pool->getNumWorkers()) {
		if (isRevisitValid(br))
			pool->submit(extractState());
		return false;
	}
	return true;
}

auto GenMCDriver::restrictAndRevisit(const WorkList::ItemT &item) -> bool
{
	/* First, appropriately restrict the worklist and the graph */
	auto &g = getExec().getGraph();
	auto *br = genmc::dyn_cast<BackwardRevisit>(&*item);
	auto stamp = g.getEventLabel(br ? br->getRev() : item->getPos())->getStamp();
	/* A weak CAS that a backward revisit made succeed reads from a write
	 * added after it: its spurious failure keeps that write (and the graph
	 * of the revisit), not just what preceded the read */
	if (const auto *wcf = genmc::dyn_cast<WeakCasFailureRevisit>(&*item))
		stamp = std::max(stamp, g.getEventLabel(wcf->getRev())->getStamp());
	restrictExecution(stamp, "restricting the graph for a revisit");
	/* Cutting the graph bypasses the incremental counter updates */
	getConsChecker().recomputeCacheCounters(getExec().getGraph());
	repairDanglingReads(g);

	pendingResume_ = nullptr;
	if (auto *fr = genmc::dyn_cast<ForwardRevisit>(&*item)) {
		/* The graph is now what it was when the event was added: its
		 * runtime state, if kept, is where the next execution resumes */
		pendingResume_ = fr->getResumePoint();
		if (pendingResume_ && --pendingResume_->pending == 0)
			std::erase(resumePoints_, pendingResume_); /* its last revisit */
		if (pendingResume_ && checksInvariants() && pendingResume_->fingerprint)
			invariants::checkResume(g, *pendingResume_->fingerprint,
						pendingResume_->threadPositions(),
						invariantContext("restricting the graph for a "
								 "resumed revisit"));
		return forwardRevisit(*fr);
	}
	if (auto *br = genmc::dyn_cast<BackwardRevisit>(&*item)) {
		return backwardRevisit(*br);
	}
	UNREACHABLE();
	return false;
}

auto GenMCDriver::handleHelpingCas(std::unique_ptr<HelpingCasLabel> hLab)
	-> HandleResult<std::monostate>
{
	VERIFY(getConf()->helper);

	/* Ensure that the helped CAS exists */
	auto &g = getExec().getGraph();
	auto *lab = genmc::dyn_cast<HelpingCasLabel>(addLabelToGraph(std::move(hLab)));
	if (!checkHelpingCasCondition(lab)) {
		blockThread(g, HelpedCASBlockLabel::create(lab->getPos()));
		return {.result = Reset{}, .count = 0U};
	}
	return {.result = std::monostate(), .count = 1U};
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

auto GenMCDriver::handleOptional(std::unique_ptr<OptionalLabel> lab) -> HandleResult<bool>
{
	auto &g = getExec().getGraph();

	if (std::any_of(g.label_begin(), g.label_end(), [&](auto &lab) {
		    auto *oLab = genmc::dyn_cast<OptionalLabel>(&lab);
		    return oLab && !oLab->isExpandable();
	    }))
		lab->setExpandable(false);

	auto *oLab = genmc::dyn_cast<OptionalLabel>(addLabelToGraph(std::move(lab)));

	/* Always expand in random exploration */
	if (inEstimationMode() || inRandomMode()) {
		oLab->setExpandable(false);
		oLab->setExpanded(true);
		return {.result = true, .count = 1U};
	}

	/* Otherwise, check whether we should expand */
	if (oLab->isExpandable()) {
		getExec().getWorkqueue().add(
			std::make_unique<OptionalForwardRevisit>(oLab->getPos()));
	}
	return {.result = false, .count = 1U}; /* not expanded in this exploration */
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

auto GenMCDriver::handleSpinStart(std::unique_ptr<SpinStartLabel> lab)
	-> HandleResult<std::monostate>
{
	auto &g = getExec().getGraph();
	auto &frontier = g.getState();
	auto loop = lab->getLoop();
	auto *stLab = addLabelToGraph(std::move(lab));

	/* Check for side-effects only if it's not the first spin-start */
	const auto *lbLab = genmc::dyn_cast<LoopBeginLabel>(g.po_imm_pred(stLab));
	if ((lbLab && lbLab->getLoop() == loop) || frontier.onSpinStart(stLab->getPos(), loop))
		return {.result = std::monostate(), .count = 1U};

	/* Spinloop detected */
	blockThreadTryMoot(SpinloopBlockLabel::create(stLab->getPos(), loop));
	return {.result = std::monostate(), .count = 1U};
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

/* An await is not a loop: an await nested in a spinloop leaves the loop
 * begin alone. It only marks the iteration as one that waited (see
 * ExecutionState::onSpinStart()) */
auto GenMCDriver::handleAwaitStart(std::unique_ptr<AwaitStartLabel> lab)
	-> HandleResult<std::monostate>
{
	getExec().getGraph().getState().onAwaitStart(lab->getPos());
	return handleDummy(std::move(lab));
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

auto GenMCDriver::areFaiZNEConstraintsSat(const FaiZNESpinEndLabel *lab) -> bool
{
	auto &g = getExec().getGraph();

	/* Check that there are no other side-effects since the previous iteration.
	 * We don't have to look for a BEGIN label since ZNE labels are always
	 * preceded by a spin-start */
	auto preds = g.po_preds(lab);
	auto ssLabIt = std::ranges::find_if(
		preds, [](auto &lab) { return genmc::isa<SpinStartLabel>(&lab); });
	VERIFY(ssLabIt != preds.end());
	const auto *ssLab = &*ssLabIt;
	for (auto i = ssLab->getIndex() + 1; i < lab->getIndex(); ++i) {
		auto *oLab = g.getEventLabel(Event(ssLab->getThread(), i));
		if (genmc::isa<WriteLabel>(oLab) && !genmc::isa<FaiWriteLabel>(oLab))
			return false;
	}

	auto wLabIt = std::ranges::find_if(
		preds, [](auto &lab) { return genmc::isa<FaiWriteLabel>(&lab); });
	VERIFY(wLabIt != preds.end());

	/* All stores in the RMW chain need to be read from at most 1 read,
	 * and there need to be no other stores that are not hb-before lab */
	const auto *wLab = genmc::dyn_cast<FaiWriteLabel>(&*wLabIt);
	for (auto &lab : g.labels()) {
		if (auto *mLab = genmc::dyn_cast<MemAccessLabel>(&lab)) {
			if (mLab->getAddr() == wLab->getAddr() && !genmc::isa<FaiReadLabel>(mLab) &&
			    !genmc::isa<FaiWriteLabel>(mLab) &&
			    !getConsChecker().getHbView(wLab).contains(mLab->getPos()))
				return false;
		}
	}
	return true;
}

auto GenMCDriver::handleFaiZNESpinEnd(std::unique_ptr<FaiZNESpinEndLabel> lab)
	-> HandleResult<std::monostate>
{
	auto &g = getExec().getGraph();

	auto *zLab = genmc::dyn_cast<FaiZNESpinEndLabel>(addLabelToGraph(std::move(lab)));
	if (!inRandomMode() && areFaiZNEConstraintsSat(zLab))
		blockThread(g, FaiZNEBlockLabel::create(zLab->getPos())); /* no moot desired */
	return {.result = std::monostate(), .count = 1U};
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

auto GenMCDriver::handleLockZNESpinEnd(std::unique_ptr<LockZNESpinEndLabel> lab)
	-> HandleResult<std::monostate>
{
	auto *zLab = addLabelToGraph(std::move(lab));
	blockThreadTryMoot(LockZNEBlockLabel::create(zLab->getPos()));
	return {.result = std::monostate(), .count = 1U};
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

auto GenMCDriver::handleDummy(std::unique_ptr<EventLabel> lab) -> HandleResult<std::monostate>
{
	addLabelToGraph(std::move(lab));
	return {.result = std::monostate(), .count = 1U};
}

auto GenMCDriver::handleLoopBegin(std::unique_ptr<LoopBeginLabel> lab)
	-> HandleResult<std::monostate>
{
	getExec().getGraph().getState().onLoopBegin(lab->getPos(), lab->getLoop());
	return handleDummy(std::move(lab));
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

/************************************************************
 ** Printing facilities
 ***********************************************************/

auto GenMCDriver::updateErrorInfoAndMaybeExit(Event pos, bool isNA, const EventDbgInfo *dbg)
	-> std::optional<VerificationError>
{
	/* If possible, update metadata */
	if (!getScheduler().inErrorReplay())
		return std::nullopt;
	if (dbg && (!isNA || Config::emitNALabels)) {
		VERIFY(!dbgInfo_.contains(pos));
		dbgInfo_[pos] = *dbg;
	}

	/* Check whether error replay is complete */
	if (bufferedError_ && getScheduler().isErrorReplayEvent(pos)) {
		auto code = bufferedError_.value().type;       /* will be erased */
		auto halt = bufferedError_.value().shouldHalt; /* will be erased */
		reportError(pos, *bufferedError_);
		return halt ? std::optional(code) : std::nullopt;
	}
	return std::nullopt;
}
auto GenMCDriver::getDbgInfo(Event pos) -> const GenMCDriver::EventDbgInfo *
{
	if (!dbgInfo_.contains(pos))
		return nullptr;
	return &dbgInfo_.at(pos);
}

void GenMCDriver::inheritDbgInfo(Event from, const BlockLabel *bLab)
{
	if (auto it = dbgInfo_.find(from); it != dbgInfo_.end())
		dbgInfo_.try_emplace(bLab->getPos(), it->second);
}
