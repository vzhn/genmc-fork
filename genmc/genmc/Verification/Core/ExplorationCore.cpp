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

#include "ExplorationCore.hpp"
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
#include "genmc/Verification/Rules/HelpingCasRules.hpp"
#include "genmc/Verification/Rules/InPlaceRevisits.hpp"
#include "genmc/Verification/Rules/KeptEventAlignment.hpp"
#include "genmc/Verification/Rules/SpinloopRules.hpp"
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

ExplorationCore::ExplorationCore(std::shared_ptr<const Config> conf, WorkSharing *pool /* = nullptr */,
			 Mode mode /* = VerificationMode{} */)
	: mode(mode), pool(pool), userConf(std::move(conf)), resumePoints_(userConf->resumePoints)
{
	/* Set up the execution context */
	consChecker = ConsistencyChecker::create(
		userConf->model, {.collectLinSpec = userConf->collectLinSpec.has_value(),
				  .checkLinSpec = userConf->checkLinSpec.has_value(),
				  .exhaustive = userConf->exhaustive,
				  .lkmmReference = userConf->lkmmReference});
	stack_ = std::make_unique<ExplorationStack>(*consChecker, execState_,
						    userConf->isDepTrackingModel);
	stack_->reset(stack_->emptyFrame());

	symmChecker = SymmetryChecker::create();
	auto hasBounder = userConf->bound.has_value();
	GENMC_DEBUG(hasBounder |= userConf->boundsHistogram;);
	if (hasBounder)
		bounder = BoundDecider::create(getConf()->boundType);

	scheduler_ = (inEstimationMode() || inRandomMode())
			     ? std::make_unique<WFRScheduler>(getConf())
			     : Scheduler::create(getConf());
	errorReplay_ = std::make_unique<ErrorReplay>(*scheduler_);

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

ExplorationCore::~ExplorationCore() = default;

static void repairRead(ExecutionGraph &g, ReadLabel *lab)
{
	auto *maxLab = g.co_max(lab->getAddr());
	lab->setRf(maxLab);
}

void ExplorationCore::repairDanglingReads(ExecutionGraph &g)
{
	for (auto i = 0; i < g.getNumThreads(); i++) {
		auto *rLab = genmc::dyn_cast<ReadLabel>(g.getLastThreadLabel(i));
		if (rLab && !rLab->getRf()) {
			repairRead(g, rLab);
			updateLabelViews(rLab);
		}
	}
}

/* The cache follows the labels added to a graph (see Scheduler::resetCacheCursors()):
 * each change of the graph other than an addition at a thread's end resets it */
void ExplorationCore::pushExecution(Execution &&e)
{
	stack_->push(std::move(e));
	getScheduler().resetCacheCursors();
}

auto ExplorationCore::invariantContext(std::string_view where) const -> invariants::Context
{
	return {.where = where,
		.dbgInfo = &errorReplay_->dbgInfo(),
		.holesAllowed = getConf()->isDepTrackingModel,
		.rmwPairsKept = !getConf()->isDepTrackingModel || getConf()->exhaustive};
}

void ExplorationCore::checkGraphInvariants(const ExecutionGraph &g, std::string_view where,
				       const invariants::RestrictionPlan *plan) const
{
	if (!checksInvariants())
		return;
	invariants::checkGraph(g, invariantContext(where), plan);
}

void ExplorationCore::checkAllocationInvariants(std::string_view where) const
{
	if (!checksInvariants())
		return;
	const auto &g = getExec().getGraph();
	invariants::checkAllocations(g, g.getState(), invariantContext(where));
}

void ExplorationCore::restrictExecution(Stamp stamp, std::string_view where)
{
	getScheduler().resetCacheCursors();
	if (!checksInvariants()) {
		stack_->restrict(stamp);
		return;
	}
	auto plan = invariants::planCutToStamp(getExec().getGraph(), stamp);
	stack_->restrict(stamp);
	checkGraphInvariants(getExec().getGraph(), where, &plan);
}

auto ExplorationCore::popExecution() -> bool
{
	getScheduler().resetCacheCursors();
	return stack_->pop();
}

void ExplorationCore::initFromState(std::unique_ptr<Execution> exec)
{
	/* A task is only taken once the previous one is done, and a done task
	 * has reported its errors (or the pool halted and no task is taken):
	 * a replay still pending here is stale (see -check-invariants, I2) */
	if (checksInvariants() && (errorReplay_->active() || errorReplay_->buffered())) {
		auto target = errorReplay_->target();
		invariants::fail("I2 (error replay)",
				 std::format("a new task is taken with an error replay pending "
					     "(target {}, buffered error: {})",
					     target ? std::format("{}", *target) : "none",
					     errorReplay_->buffered().has_value()),
				 stack_->empty() ? static_cast<const ExecutionGraph *>(nullptr)
						   : &getExec().getGraph(),
				 invariantContext("taking a task from the pool"));
	}
	checkGraphInvariants(*exec->graph, "taking a task from the pool");

	/* A replay that collects an error's metadata belongs to the executions
	 * being replaced: drop it, or the next schedule would replay to an event
	 * of the old graph. (It is cut short only when the pool halts, and then
	 * the error is reported by the worker that halted it.) */
	errorReplay_->stop();
	resumePoints_.clearPending();

	stack_->reset({std::move(exec->graph), LocalQueueT(), std::move(exec->choices)});
	getScheduler().resetCacheCursors();
}

auto ExplorationCore::extractState() -> std::unique_ptr<ExplorationCore::Execution>
{
	auto og = getExec().getGraph().clone();
	checkGraphInvariants(*og, "copying the graph for another worker");
	return std::make_unique<Execution>(ExplorationCore::Execution(
		std::move(og), LocalQueueT(), ChoiceMap(getExec().getChoiceMap())));
}

void ExplorationCore::maybeCutGraph(const EventLabel *lab)
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

void ExplorationCore::restartExecution()
{
	resumePoints_.clearPending();
	stack_->reset(stack_->emptyFrame());
	getScheduler().resetCacheCursors();
}

auto ExplorationCore::done() -> bool
{
	/* Repeat a run whose thread's kept events were moved: from its start
	 * (the events added since are cut), so that every structure built during
	 * the run sees the new positions. Or repeat a run that was inconsistent
	 * after an event went before kept ones, checking after each such event
	 * (the work it added is cut) */
	switch (keptRuns_.endRun(isHalting())) {
	case KeptEventRuns::Repeat::AfterRelocation: {
		++result.relocationReruns;
		static constexpr auto kMaxRelocations = 64U;
		ERROR_ON(keptRuns_.relocationsInRun() > kMaxRelocations,
			 "Internal error: a thread's kept events do not settle\nPlease submit a "
			 "bug report to {}\n",
			 PACKAGE_BUGREPORT);
		getExec().getWorkqueue().truncate(keptRuns_.runStartWork());
		restrictExecution(keptRuns_.runStartStamp(),
				  "restricting the graph to rerun after a relocation");
		repairDanglingReads(getExec().getGraph());
		return false;
	}
	case KeptEventRuns::Repeat::Eagerly:
		++result.keptCheckReruns;
		getExec().getWorkqueue().truncate(keptRuns_.runStartWork());
		restrictExecution(keptRuns_.runStartStamp(),
				  "restricting the graph to rerun with eager checks");
		repairDanglingReads(getExec().getGraph());
		return false;
	case KeptEventRuns::Repeat::None:
		break;
	}

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
auto ExplorationCore::visitRevisitedGraph(const Revisit &item) -> bool
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

auto ExplorationCore::isHalting() const -> bool
{
	auto *pool = getThreadPool();
	return shouldHalt || (pool && pool->shouldHalt());
}

void ExplorationCore::halt(VerificationError status)
{
	shouldHalt = true;
	result.status = status;
	if (getThreadPool())
		getThreadPool()->halt();
}

/************************************************************
 ** Scheduling methods
 ***********************************************************/

void ExplorationCore::blockThreadTryMoot(std::unique_ptr<BlockLabel> bLab)
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

auto ExplorationCore::checkPendingKept(const ExecutionGraph &g) -> bool
{
	if (!keptRuns_.takeDeferredCheck())
		return true;
	if (getConsChecker().isConsistent(g))
		return true;
	keptRuns_.repeatEagerly();
	moot();
	return false;
}

auto ExplorationCore::isExecutionValid(const EventLabel *lab) -> bool
{
	const auto *rLab = genmc::dyn_cast<ReadLabel>(lab);
	return (!getConf()->symmetryReduction || getSymmChecker().isSymmetryOK(lab)) &&
	       ((rLab && rLab->getRf() == getExec().getGraph().co_max(rLab->getAddr()) &&
		 getConsChecker().isCoMaxReadConsistent(rLab)) ||
		getConsChecker().isConsistent(lab)) &&
	       !partialExecutionExceedsBound();
}

auto ExplorationCore::isRevisitValid(const Revisit &revisit) -> bool
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

auto ExplorationCore::isExecutionDrivenByGraph(Event curr) -> bool
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

auto ExplorationCore::alignKeptEvents(Event curr, const EventLabel *lab) -> bool
{
	using Kind = kept_events::Alignment::Kind;

	auto sub = curr.index - currKeyBase.index;
	/* No such run: it is dropped, and the event being aligned is not added
	 * (handleLoad(), handleStore() and handleThreadFinish() reset their
	 * instruction; the other kinds go into the hole, harmlessly). Before,
	 * the run went on with the event: a second event of the instance,
	 * placed in coherence against the kept ones and so contradicting it,
	 * and the revisits and warnings computed from that graph */
	auto dropRun = [&] {
		++result.relocationsLost;
		keptRuns_.dropRun();
		moot();
	};

	auto alignment = kept_events::align(getExec().getGraph(), curr, *currKey, sub);
	switch (alignment.kind) {
	case Kind::Own:
		return true;
	case Kind::MissingKey:
		checkReplayKey(lab, sub); /* a missing key is an error still */
		return false;
	case Kind::ShiftAndDrop:
		relocateExecution(curr.thread, curr.index, 1);
		dropRun();
		return false;
	case Kind::OpenGap:
		/* (Doubling on repeats; the surplus is closed later) */
		relocateAndRerun(curr.thread, curr.index,
				 1 << std::min(keptRuns_.relocationsInRun(), 10U));
		return false;
	case Kind::FillHole:
		return false;
	case Kind::Drop:
		dropRun();
		return false;
	case Kind::FillBeforeKept:
		keptRuns_.addedBeforeKept();
		return false;
	case Kind::MoveHere:
		relocateAndRerun(curr.thread, alignment.from, curr.index - alignment.from);
		return true;
	}
	UNREACHABLE();
	return false;
}

void ExplorationCore::relocateExecution(int thread, int from, int delta)
{
	getExec().getGraph().relocateThread(thread, from, delta);
	getExec().getWorkqueue().relocateThread(thread, from, delta);
	getExec().getChoiceMap().relocateThread(thread, from, delta);
	checkGraphInvariants(getExec().getGraph(), "relocating a thread's events");
}

void ExplorationCore::relocateAndRerun(int thread, int from, int delta)
{
	relocateExecution(thread, from, delta);
	keptRuns_.repeatAfterRelocation();
	moot();
}

void ExplorationCore::assignEventKey(EventLabel *lab) const
{
	if (currKey && lab->getThread() == currKeyBase.thread &&
	    lab->getIndex() > currKeyBase.index)
		lab->setKey(currKey, lab->getIndex() - currKeyBase.index);
	else
		lab->setKey(nullptr, 0);
}

void ExplorationCore::checkReplayKey(const EventLabel *lab, std::uint32_t sub) const
{
	if (lab->getKey() && *lab->getKey() == *currKey && lab->getKeySub() == sub)
		return;
	ERROR("Internal error: replaying {} reuses the label of another instruction instance "
	      "({} #{}, now {} #{}): {}\nPlease submit a bug report to {}\n",
	      lab->getPos(), lab->getKey() ? lab->getKey()->toString() : "none",
	      lab->getKeySub(), currKey->toString(), sub, *lab, PACKAGE_BUGREPORT);
}

auto ExplorationCore::executionExceedsBound(BoundCalculationStrategy strategy) const -> bool
{
	if (!getConf()->bound.has_value() || inEstimationMode())
		return false;

	return bounder->doesExecutionExceedBound(getExec().getGraph(), *getConf()->bound, strategy);
}

auto ExplorationCore::partialExecutionExceedsBound() const -> bool
{
	return executionExceedsBound(BoundCalculationStrategy::Slacked);
}

auto ExplorationCore::addLabelToGraph(std::unique_ptr<EventLabel> lab) -> EventLabel *
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

void ExplorationCore::updateLabelViews(EventLabel *lab)
{
	getConsChecker().updateMMViews(lab);
	if (!getConf()->symmetryReduction)
		return;

	getSymmChecker().updatePrefixWithSymmetries(lab);
}

auto ExplorationCore::checkThreadLocalEnd(const EventLabel *lab, const ThreadFinishLabel *eLab)
	-> std::optional<VerificationError>
{
	return reportIfViolated(error_rules::threadLocalAfterEnd(getConsChecker(), lab, eLab));
}

auto ExplorationCore::checkEndedThreadLocals(const ThreadFinishLabel *eLab)
	-> std::optional<VerificationError>
{
	if (getConf()->disableRaceDetection || inEstimationMode())
		return {};
	return reportIfViolated(
		error_rules::endedThreadLocal(getConsChecker(), getExec().getGraph(), eLab));
}

auto ExplorationCore::checkForRaces(const EventLabel *lab) -> std::optional<VerificationError>
{
	if (inEstimationMode())
		return {};

	/* An access to the thread-local of a thread that has ended (see also
	 * checkEndedThreadLocals()) */
	const auto &g = getExec().getGraph();
	if (auto owner = error_rules::threadLocalOwner(lab); owner != 0) {
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
			const auto *cLab = cg.getEventLabel(lab->getPos());
			err = getConsChecker().checkErrors(cLab, racyLab);
			/* The model's race check takes each thread's last access to
			 * the location for one added before LAB. Not so if LAB fills a
			 * hole: a kept access after it can be after LAB in hb, and one
			 * after a hole in its own thread is not reached yet (its thread
			 * may move or drop it). LAB is checked against the others,
			 * both ways; the rest are checked once the execution is
			 * complete (see GenMCDriver::checkExhaustiveRace()) (#179) */
			if (err == VerificationError::VE_RaceNotAtomic) {
				auto race = error_rules::hbRaceWith(getConsChecker(), cg, cLab, true);
				err = race ? err : std::nullopt;
				racyLab = race ? race->racyLab : nullptr;
			}
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

auto ExplorationCore::getReadRetValue(const ReadLabel *rLab) -> HandleResult<SVal>
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

/* The rfs getRfsApproximation() drops are the writes whose successful RMW
 * would violate atomicity (a settled RMW reads them already). A weak CAS can
 * still read such a write and fail spuriously: that execution has no write to
 * violate anything, and no other path explores it */
void ExplorationCore::pushWeakCasFailuresOnDroppedRfs(ReadLabel *lab,
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

void ExplorationCore::maybePushWeakCasFailure(const CasReadLabel *cLab)
{
	/* Pushes the spurious-failure option of a *successful* weak CAS;
	 * callers must have already established success. */
	if (!cLab->isWeak() || cLab->failsSpuriously() || !cLab->getRf())
		return;
	getExec().getWorkqueue().add(
		std::make_unique<WeakCasFailureRevisit>(cLab->getPos(), cLab->getRf()->getPos()));
}

auto ExplorationCore::getInvalidFreeReason(SAddr loc, StorageDuration sdur) const
	-> std::optional<std::string>
{
	return error_rules::invalidFreeReason(getExec().getGraph().getState(), loc, sdur);
}

auto ExplorationCore::checkAccessValidity(Event pos, const AAccess &access)
	-> std::optional<VerificationError>
{
	return reportIfViolated(error_rules::invalidAccess(getExec().getGraph().getState(),
							   !getConf()->disableStaticValidityChecks,
							   pos, access));
}

auto ExplorationCore::checkInitializedMem(const ReadLabel *rLab) -> std::optional<VerificationError>
{
	/* Allow frontends to skip uninit/msa checks */
	if (getConf()->disableInitializationChecks)
		return {};
	return reportIfViolated(error_rules::uninitializedRead(getConsChecker(), rLab));
}

auto ExplorationCore::checkInitializedMem(const WriteLabel *wLab) -> std::optional<VerificationError>
{
	return reportIfViolated(error_rules::invalidUnlock(wLab));
}

auto ExplorationCore::getErrorPrefix(const ExecutionGraph &g) const
	-> std::unique_ptr<ExecutionGraph>
{
	/* An error found as a read is added (see handleLoad()) comes before the
	 * read has an rf: the error is on the prefix before the read, and the
	 * read, taken as reading the initializer, can make the graph
	 * inconsistent (#221). Such a read is checked as not yet added. So is a
	 * store checked before it is placed in coherence (an access to
	 * non-allocated memory): the model has no coherence position for it */
	auto pending = std::views::filter(g.thr_ids(), [&](auto t) {
		const auto *lab = g.getLastThreadLabel(t);
		if (const auto *rLab = genmc::dyn_cast<ReadLabel>(lab))
			return !rLab->getRf();
		const auto *wLab = genmc::dyn_cast<WriteLabel>(lab);
		return wLab && !wLab->isInCo();
	});
	if (std::ranges::empty(pending))
		return nullptr;

	auto v = g.getViewFromStamp(g.getMaxStamp());
	for (auto t : pending)
		v->setMax(g.getLastThreadLabel(t)->getPos().prev());
	return g.getCopyUpTo(*v);
}

auto ExplorationCore::isErrorGraphConsistent(const ExecutionGraph &g) const -> bool
{
	auto prefix = getErrorPrefix(g);
	return getConsChecker().isConsistentWithSectionsClosed(prefix ? *prefix : g);
}

auto ExplorationCore::reportIfViolated(const std::optional<ErrorDetails> &violation)
	-> std::optional<VerificationError>
{
	if (!violation)
		return {};
	reportError(*violation->pos, *violation);
	return {violation->type};
}

auto ExplorationCore::checkFinalAnnotations(const WriteLabel *wLab) -> std::optional<VerificationError>
{
	if (!getConf()->finalWrite)
		return {};
	return reportIfViolated(
		error_rules::finalWriteViolation(getConsChecker(), getExec().getGraph(), wLab));
}

auto ExplorationCore::checkIPRValidity(const ReadLabel *rLab) -> std::optional<VerificationError>
{
	if (!getConf()->ipr)
		return {};
	auto violation = error_rules::iprUnorderedWrites(getExec().getGraph(), rLab);
	if (violation)
		getResult().iprError = true;
	return reportIfViolated(violation);
}

auto ExplorationCore::checkBlindRMWWellFormedness(const ReadLabel *rLab) -> bool
{
	if (!getConf()->blindRMWReduction)
		return true;
	return !reportIfViolated(
		error_rules::blindRMWOrderObserved(getConsChecker(), getExec().getGraph(), rLab));
}

auto ExplorationCore::checkBlindRMWWellFormedness(const WriteLabel *sLab) -> bool
{
	if (!getConf()->blindRMWReduction)
		return true;
	return !reportIfViolated(
		error_rules::blindRMWReorderedWith(getConsChecker(), getExec().getGraph(), sLab));
}

void ExplorationCore::unblockWaitingHelping(const WriteLabel *lab)
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

auto ExplorationCore::checkHelpingCasCondition(const HelpingCasLabel *hLab) -> bool
{
	auto &g = getExec().getGraph();
	auto helped = helping_cas::helpedCases(g, *hLab);
	if (std::ranges::any_of(helped, [&](auto *rLab) {
		    return !helping_cas::seesStoresBeforeHelped(g, getConsChecker(), *rLab, *hLab);
	    }))
		ERROR("Helped/Helping CAS annotation error! "
		      "Not all stores before helped-CAS are visible to helping-CAS!");
	return !helped.empty();
}

auto ExplorationCore::findConsistentRf(ReadLabel *rLab, std::vector<EventLabel *> &rfs) -> EventLabel *
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

auto ExplorationCore::isPlacementConsistent(const WriteLabel *wLab) const -> bool
{
	/* A write placed last in coherence keeps a consistent graph consistent
	 * (extensibility), which findConsistentCo() relies on. The write of an
	 * RMW goes right after the write its read part read, which need not be
	 * the last one, and its coe edges can then close a cycle that the checks
	 * of the read part could not see: under LKMM through prop (R ->ctrl W
	 * ->coe W' ->po-rel W'' ->rfe R), under TSO through tso (W ->mo W' ->po
	 * RMW ->mo W'' ->po W, tests/correct/infr/tso-rmw-placement). Which
	 * models need the check: see rmwPlacementNeedsCheck() */
	const auto &g = *wLab->getParent();
	return !rmwPlacementNeedsCheck(getConf()->model) || g.co_max(wLab->getAddr()) == wLab ||
	       getConsChecker().isConsistent(wLab);
}

auto ExplorationCore::findConsistentCo(WriteLabel *wLab, std::vector<EventLabel *> &cos) -> EventLabel *
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

auto ExplorationCore::handleThreadKill(std::unique_ptr<ThreadKillLabel> kLab)
	-> HandleResult<std::monostate>
{
	addLabelToGraph(std::move(kLab));
	return {.result = std::monostate{}, .count = 1U};
}

auto ExplorationCore::getSymmetricTidSR(const ThreadCreateLabel *tcLab,
				    const ThreadInfo &childInfo) const -> int
{
	if (!getConf()->symmetryReduction)
		return -1;
	return symmetricPredecessor(getExec().getGraph(), tcLab, childInfo);
}

auto ExplorationCore::handleThreadCreate(std::unique_ptr<ThreadCreateLabel> tcLab) -> HandleResult<int>
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

auto ExplorationCore::checkSymmetricJoinOrder(const ThreadJoinLabel *jLab)
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

auto ExplorationCore::handleThreadJoin(std::unique_ptr<ThreadJoinLabel> lab) -> HandleResult<SVal>
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

auto ExplorationCore::handleThreadFinish(std::unique_ptr<ThreadFinishLabel> eLab)
	-> HandleResult<std::monostate>
{
	auto &g = getExec().getGraph();

	/* The run was dropped while aligning this event (alignKeptEvents()) */
	if (keptRuns_.runDropped())
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

auto ExplorationCore::handleFence(std::unique_ptr<FenceLabel> fLab) -> HandleResult<std::monostate>
{
	auto *lab = addLabelToGraph(std::move(fLab));
	getExec().getGraph().getState().onFence(lab->getPos(), lab->isAtLeastRelease());
	return {.result = std::monostate(), .count = 1U};
}

void ExplorationCore::checkReconsiderFaiSpinloop(const MemAccessLabel *lab)
{
	auto &g = getExec().getGraph();

	/* The blocked threads spin again: their next iteration ends here */
	for (const auto *eLab : spin_rules::faiSpinloopsBrokenBy(g, getConsChecker(), *lab)) {
		auto pos = eLab->getPos();
		auto key = eLab->getSharedKey();
		auto keySub = eLab->getKeySub();
		unblockThread(g, pos);
		auto zLab = FaiZNESpinEndLabel::create(pos);
		zLab->setKey(std::move(key), keySub);
		addLabelToGraph(std::move(zLab));
	}
}

auto ExplorationCore::getPrefixView(const EventLabel *lab) const -> const VectorClock &
{
	return prefixView(getConsChecker(), lab);
}

auto ExplorationCore::getRevisitPrefixView(const EventLabel *lab) const
	-> std::unique_ptr<VectorClock>
{
	return revisitPrefixView(getConsChecker(), scopesCtrlDeps(), lab);
}

auto ExplorationCore::getBackwardRevisits() const -> BackwardRevisits
{
	return {getExec().getGraph(), getConsChecker(),
		{.depTracking = getConf()->isDepTrackingModel,
		 .disableBAM = getConf()->disableBAM,
		 .scopesCtrlDeps = scopesCtrlDeps(),
		 .exhaustive = getConf()->exhaustive,
		 .exhaustiveIMM = getConf()->exhaustive && getConf()->model == ModelType::IMM}};
}

auto ExplorationCore::getRfReductions() const -> RfReductions
{
	return {getExec().getGraph(), getConsChecker(), getSymmChecker(),
		{.symmetryReduction = getConf()->symmetryReduction,
		 .disableBAM = getConf()->disableBAM,
		 .blindRMWReduction = getConf()->blindRMWReduction,
		 .exhaustive = getConf()->exhaustive}};
}

auto ExplorationCore::getChoiceCandidates() -> ChoiceCandidates
{
	return {getExec().getGraph(), getConsChecker(),
		{.depTracking = getConf()->isDepTrackingModel,
		 .scopesCtrlDeps = scopesCtrlDeps(),
		 .exhaustive = getConf()->exhaustive,
		 .exhaustiveIMM = getConf()->exhaustive && getConf()->model == ModelType::IMM}};
}

auto ExplorationCore::getRfsApproximation(ReadLabel *lab) -> std::vector<EventLabel *>
{
	return getChoiceCandidates().rfs(lab);
}

auto ExplorationCore::getRevisitableApproximation(WriteLabel *sLab) -> std::vector<ReadLabel *>
{
	return getChoiceCandidates().revisitableReads(sLab);
}

auto ExplorationCore::pickRandomRf(ReadLabel *rLab, std::vector<EventLabel *> &stores) -> EventLabel *
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

auto ExplorationCore::checkForMixedSize(MemAccessLabel *lab) -> std::optional<VerificationError>
{
	return reportIfViolated(error_rules::mixedSizeAccess(lab));
}

auto ExplorationCore::handleLoad(std::unique_ptr<ReadLabel> rLab, std::optional<SVal> oldVal)
	-> HandleResult<SVal>
{
	auto &g = getExec().getGraph();
	auto &frontier = g.getState();

	/* The run was dropped while aligning this event (alignKeptEvents()) */
	if (keptRuns_.runDropped())
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
	/* The read fills a hole, and the events a revisit kept after it leave
	 * it no coherent write (-exhaustive under IMM, see getCoherentStores()),
	 * or none an RMW can read: the write a kept later RMW reads, or a kept
	 * later read of its thread (see ChoiceCandidates::rfs()). No completion
	 * of the graph is consistent. The read reads from the last write, for
	 * the graph to stay well formed until the run is dropped */
	if (stores.empty() && getConf()->isDepTrackingModel) {
		repairRead(g, lab);
		moot();
		return {.result = Invalid{}, .count = 1U};
	}
	VERIFY(!stores.empty());
	GENMC_DEBUG(LOG(VerbosityLevel::Debug3, "Rfs: {}", stores););
	if (inVerificationMode())
		pushWeakCasFailuresOnDroppedRfs(lab, stores);
	getRfReductions().filter(lab, stores);
	GENMC_DEBUG(LOG(VerbosityLevel::Debug3, "Rfs (optimized): {}", stores););

	const EventLabel *rf = nullptr;

	if (inEstimationMode() || inRandomMode()) {
		if (inEstimationMode())
			getExec().getChoiceMap().update(lab, stores);
		getChoiceCandidates().dropAtomicityViolations(lab, stores);
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

auto ExplorationCore::handleNALoad(Event pos, SAddr loc, ASize size, const EventDeps &deps)
	-> ExplorationCore::NALoadResult
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

auto ExplorationCore::handleNAStore(Event pos, SAddr loc, ASize size, std::optional<SVal> val,
				const EventDeps &deps) -> ExplorationCore::NAStoreResult
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

auto ExplorationCore::pickRandomCo(WriteLabel *sLab, std::vector<EventLabel *> &cos) -> EventLabel *
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

void ExplorationCore::calcCoOrderings(WriteLabel *lab, const std::vector<EventLabel *> &cos)
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

auto ExplorationCore::saveResumePoint(Event pos) -> std::shared_ptr<ResumePoint>
{
	/* Only a verification run resumes: estimation and random runs do not
	 * revisit, an error replay collects its metadata from the start,
	 * relocated kept events (LKMM) move within a run, replay keys count
	 * from the start, and labels added from the instruction cache have no
	 * runtime state behind them */
	if (!resumePoints_.hasHook() || getConf()->resumePoints == 0 || !inVerificationMode() ||
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
	auto rp = resumePoints_.save(pos);
	if (rp && checksInvariants())
		rp->fingerprint = invariants::fingerprint(g, pos);
	return rp;
}

auto ExplorationCore::handleStore(std::unique_ptr<WriteLabel> wLab, std::optional<SVal> oldVal)
	-> HandleResult<bool>
{
	auto &g = getExec().getGraph();

	/* The run was dropped while aligning this event (alignKeptEvents()) */
	if (keptRuns_.runDropped())
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

auto ExplorationCore::handleMalloc(Event pos, ASize size, uint64_t alignment, StorageDuration sdur,
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

auto ExplorationCore::handleRetire(Event pos, SAddr loc, const EventDeps &deps)
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

auto ExplorationCore::handleFree(Event pos, SAddr loc, StorageDuration sdur, const EventDeps &deps)
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

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto ExplorationCore::getPreviousVisibleAccessLabel(
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

void ExplorationCore::mootExecutionIfFullyBlocked(EventLabel *bLab)
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

auto ExplorationCore::handleBlock(std::unique_ptr<BlockLabel> lab) -> HandleResult<std::monostate>
{
	/* Call addLabelToGraph first to cache the label (not before kept
	 * events, where blockThread() puts the block after them) */
	if (!getExec().getGraph().hasEventsAfter(lab->getThread(), lab->getIndex()))
		addLabelToGraph(lab->clone());
	blockThreadTryMoot(std::move(lab));
	return {.result = std::monostate(), .count = 1U};
}

void ExplorationCore::initiateErrorReplay(const ErrorDetails &details)
{
	auto &g = getExec().getGraph();

	/* Initiate an exploration, and block the current one if it's a hard error */
	getExec().getWorkqueue().add(std::make_unique<ReplayForwardRevisit>(
		std::ranges::begin(g.rlabels())->getPos(), details));
	if (details.shouldHalt)
		moot();
}

void ExplorationCore::haltErrorReplay()
{
	errorReplay_->stop();

	/* The error-collecting execution finished; we have to drop it */
	moot();
}

void ExplorationCore::reportError(Event pos, const ErrorDetails &details)
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
		/* (Without the events still pending, see getErrorPrefix()) */
		auto prefix = getErrorPrefix(g);
		const auto &eg = prefix ? *prefix : g;
		if (getConf()->exhaustive && getConf()->model == ModelType::IMM) {
			if (!isExecutionConsistent(g)) {
				moot();
				return;
			}
		} else if (keptRuns_.takeAddedBeforeKept()) {
			if (!getConsChecker().isConsistent(eg)) {
				moot();
				return;
			}
		}
		if (!checkPendingKept(eg))
			return;
	}

	/* An error inside an open read-side critical section: the unlock the
	 * thread adds next orders the section's events against the grace
	 * periods (LKMM's rcu-order), and can make the graph inconsistent. The
	 * error is then on a prefix no execution has (#201;
	 * proofs/alloy/open-section.als): the graph is checked with its open
	 * sections closed, and the run dropped as it would be at the unlock
	 * (see GenMCDriver::handleLkmmFence()). A misuse of RCU (an unlock
	 * without a lock, a grace period inside a section) is reported as it
	 * is: the model has nothing to say about such a graph */
	const auto misusesRcu = details.type == VerificationError::VE_InvalidRcu;
	if (openSectionsNeedCheck(getConf()->model) && !misusesRcu && !scheduler.inErrorReplay() &&
	    !isErrorGraphConsistent(g)) {
		moot();
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

	/* The error is reported: its graph must be a prefix of an execution,
	 * consistent with the events its threads must add next (I6) */
	if (checksInvariants() && !misusesRcu && !scheduler.inErrorReplay())
		invariants::checkErrorConsistency(g, isErrorGraphConsistent(g),
						  invariantContext("reporting an error"));

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

	result.message += errorReport(g, details, errorReplay_->dbgInfo(), getConf()->printErrorTrace);

	/* Dump the graph into a file (DOT format) */
	if (!getConf()->dotFile.empty())
		dotPrintToFile(getConf()->dotFile, errLab,
			       errLab ? getPrefixView(errLab).clone() : nullptr, details.racyLab,
			       details.racyLab ? getPrefixView(details.racyLab).clone() : nullptr,
			       &getConsChecker(), errorReplay_->dbgInfo(),
			       getConf()->dotPrintOnlyClientEvents);

	/* Stop the error-collecting execution */
	haltErrorReplay();

	/* If this was a hard error, stop altogether */
	if (details.shouldHalt)
		halt(details.type);
}

auto ExplorationCore::reportWarningOnce(Event pos, VerificationError wcode,
				    const EventLabel *racyLab /* = nullptr */) -> bool
{
	/* If the warning has been seen before, only report it if it's an error */
	auto up = error_rules::wwRaceUpgrade(getExec().getGraph(), pos, wcode,
					     getConf()->symmetryReduction, getConf()->ipr);
	if (up.upgrade) {
		getResult().iprError = up.iprError;
		getResult().inferredSymmetryError = up.inferredSymmetryError;
	}
	auto upgradeWarning = up.upgrade;
	const auto &msg = up.msg;
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

auto ExplorationCore::checkBarrierWellFormedness(BIncFaiWriteLabel *sLab) -> bool
{
	if (getConf()->disableBAM)
		return true;
	return !reportIfViolated(
		error_rules::barrierNotWellFormed(getConsChecker(), getExec().getGraph(), sLab));
}

auto ExplorationCore::tryOptimizeBarrierRevisits(BIncFaiWriteLabel *sLab,
					     std::vector<ReadLabel *> & /*loads*/) -> bool
{
	if (getConf()->disableBAM)
		return false;

	if (!checkBarrierWellFormedness(sLab) || !isLastInBarrierRound(sLab))
		return true;

	/* Finally, revisit in place */
	auto &g = getExec().getGraph();
	for (auto pos : in_place::barrierWaits(*sLab))
		revisitInPlace(*getBackwardRevisits().construct(g.getReadLabel(pos), sLab));
	return true;
}

void ExplorationCore::tryOptimizeIPRs(const WriteLabel *sLab, std::vector<ReadLabel *> &loads)
{
	if (!getConf()->ipr)
		return;

	std::vector<ReadLabel *> toIPR;
	std::erase_if(loads, [&](auto *rLab) {
		auto blocked = in_place::blocksOnAnnotation(*rLab);
		if (blocked)
			toIPR.push_back(rLab);
		return blocked;
	});

	for (auto *rLab : toIPR)
		revisitInPlace(*getBackwardRevisits().construct(rLab, sLab));

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

auto ExplorationCore::removeCASReadIfBlocks(const ReadLabel *rLab, const EventLabel *sLab) -> bool
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

void ExplorationCore::checkReconsiderReadOpts(const WriteLabel *sLab)
{
	auto &g = getExec().getGraph();
	for (auto i = 0; i < g.getNumThreads(); i++) {
		auto *bLab = genmc::dyn_cast_if_present<ReadOptBlockLabel>(g.getLastThreadLabel(i));
		if (!bLab || bLab->getAddr() != sLab->getAddr())
			continue;
		unblockThread(g, bLab->getPos());
	}
}

void ExplorationCore::optimizeUnconfirmedRevisits(const WriteLabel *sLab,
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

auto ExplorationCore::tryOptimizeRevisits(WriteLabel *sLab, std::vector<ReadLabel *> &loads) -> bool
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

void ExplorationCore::revisitInPlace(const BackwardRevisit &br)
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

auto ExplorationCore::getExhaustiveConsistency() -> ExhaustiveConsistency
{
	return ExhaustiveConsistency(getConsChecker(), [this](EventLabel *lab) { updateLabelViews(lab); });
}

auto ExplorationCore::recomputeViews(ExecutionGraph &g) -> bool
{
	return getExhaustiveConsistency().recomputeViews(g);
}

auto ExplorationCore::isGraphConsistent(const ExecutionGraph &g) -> bool
{
	return getExhaustiveConsistency().isGraphConsistent(g);
}

auto ExplorationCore::isExecutionConsistent(const ExecutionGraph &g) -> bool
{
	return getExhaustiveConsistency().isExecutionConsistent(g);
}

void ExplorationCore::checkWithViewsAnew(const std::function<void(const ExecutionGraph &)> &check)
{
	getExhaustiveConsistency().checkWithViewsAnew(getExec().getGraph(), check);
}

auto ExplorationCore::copyGraph(const BackwardRevisit *br, VectorClock *v) const
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
	/* -exhaustive under IMM: the events a revisit keeps without their
	 * prefix (see BackwardRevisits::keepValueDependencies()) have cached
	 * prefix views that name events the copy does not have.
	 * backwardRevisit() computes the views anew, which drops them; they go
	 * now, so that no check or rule sees them in between (#181) */
	if (getConf()->exhaustive && getConf()->model == ModelType::IMM)
		ExhaustiveConsistency::dropPrefixViews(*og);
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

void ExplorationCore::calcRevisits(WriteLabel *sLab)
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
		auto revisits = getBackwardRevisits();
		auto br = revisits.construct(rLab, sLab);
		/* -exhaustive: every revisit whose graph is well formed and can be
		 * consistent (the visited set, not maximality, bounds the exploration) */
		if (getConf()->exhaustive) {
			if (br->getViewNoRel() && revisits.isRevisitViewClosed(*br) &&
			    !revisits.keepsConflictingRMW(*br))
				getExec().getWorkqueue().add(std::move(br));
			continue;
		}
		if (revisits.breaksCoherenceOfKeptLater(*br))
			continue;
		if (!revisits.isMaximalExtension(*br))
			break;

		getExec().getWorkqueue().add(std::move(br));
	}
}

auto ExplorationCore::completeRevisitedRMW(const ReadLabel *rLab) -> WriteLabel *
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
auto ExplorationCore::updateKeptRMWWrite(const ReadLabel *rLab) -> WriteLabel *
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

auto ExplorationCore::revisitWrite(const WriteForwardRevisit &ri) -> bool
{
	auto &g = getExec().getGraph();
	auto *wLab = g.getWriteLabel(ri.getPos());
	VERIFY(wLab);

	wLab->moveCo(g.getEventLabel(ri.getPred()));
	calcRevisits(wLab);
	return !violatesAtomicity(wLab);
}

auto ExplorationCore::revisitOptional(const OptionalForwardRevisit &oi) -> bool
{
	auto &g = getExec().getGraph();
	auto *oLab = genmc::dyn_cast<OptionalLabel>(g.getEventLabel(oi.getPos()));

	VERIFY(oLab);
	oLab->setExpandable(false);
	oLab->setExpanded(true);
	return true;
}

auto ExplorationCore::revisitRead(const Revisit &ri) -> bool
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

auto ExplorationCore::forwardRevisit(const ForwardRevisit &fr) -> bool
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
		errorReplay_->start(genmc::isa<BlockLabel>(errLab) ? errLab->getPos().prev()
				    : bLab && bLab->getCreate()    ? bLab->getCreate()->getPos()
								   : errLab->getPos(),
				    rr->getDetails());
		return true;
	}
	if (const auto *wcf = genmc::dyn_cast<WeakCasFailureRevisit>(&fr))
		return revisitWeakCasFailure(*wcf);
	const auto *ri = genmc::dyn_cast<ReadForwardRevisit>(&fr);
	VERIFY(ri);
	return revisitRead(*ri);
}

auto ExplorationCore::revisitWeakCasFailure(const WeakCasFailureRevisit &ri) -> bool
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

auto ExplorationCore::backwardRevisit(const BackwardRevisit &br) -> bool
{
	auto &g = getExec().getGraph();

	/* Recalculate the view because some B labels might have been
	 * removed */
	auto keptRMWReads = false;
	auto v = getBackwardRevisits().revisitView(g.getReadLabel(br.getPos()),
						   g.getWriteLabel(br.getRev()), &keptRMWReads);
	if (!v) {
		VERIFY(getConf()->exhaustive);
		return false;
	}

	auto og = copyGraph(&br, &*v);
	auto cmap = ChoiceMap(getExec().getChoiceMap());
	cmap.cut(*v);

	keptRMWReads |= getExec().keptRMWReads;
	pushExecution({std::move(og), LocalQueueT(), std::move(cmap)});
	getExec().keptRMWReads = keptRMWReads;

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

auto ExplorationCore::restrictAndRevisit(const WorkList::ItemT &item) -> bool
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
	repairDanglingReads(g);
	if (checksInvariants()) {
		using invariants::RevisitTarget;
		const auto kind = genmc::isa<WriteForwardRevisit>(&*item) ? RevisitTarget::Write
				  : genmc::isa<ReadForwardRevisit>(&*item) || br ||
						  genmc::isa<WeakCasFailureRevisit>(&*item)
					  ? RevisitTarget::Read
					  : RevisitTarget::Any;
		invariants::checkRevisitTarget(
			g, item->getPos(), kind, br ? std::optional(br->getRev()) : std::nullopt,
			invariantContext("restricting the graph for a revisit"));
	}

	resumePoints_.clearPending();
	if (auto *fr = genmc::dyn_cast<ForwardRevisit>(&*item)) {
		/* The graph is now what it was when the event was added: its
		 * runtime state, if kept, is where the next execution resumes */
		resumePoints_.resumeAt(fr->getResumePoint());
		const auto &rp = resumePoints_.pending();
		if (rp && checksInvariants() && rp->fingerprint)
			invariants::checkResume(g, *rp->fingerprint, rp->threadPositions(),
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

auto ExplorationCore::handleHelpingCas(std::unique_ptr<HelpingCasLabel> hLab)
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

auto ExplorationCore::handleOptional(std::unique_ptr<OptionalLabel> lab) -> HandleResult<bool>
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

auto ExplorationCore::handleSpinStart(std::unique_ptr<SpinStartLabel> lab)
	-> HandleResult<std::monostate>
{
	auto &g = getExec().getGraph();
	auto &frontier = g.getState();
	auto loop = lab->getLoop();
	auto *stLab = addLabelToGraph(std::move(lab));

	/* Check for side-effects only if it's not the first spin-start */
	if (spin_rules::startsFirstIteration(g, *genmc::cast<SpinStartLabel>(stLab)) ||
	    frontier.onSpinStart(stLab->getPos(), loop))
		return {.result = std::monostate(), .count = 1U};

	/* Spinloop detected */
	blockThreadTryMoot(SpinloopBlockLabel::create(stLab->getPos(), loop));
	return {.result = std::monostate(), .count = 1U};
}

/* An await is not a loop: an await nested in a spinloop leaves the loop
 * begin alone. It only marks the iteration as one that waited (see
 * ExecutionState::onSpinStart()) */
auto ExplorationCore::handleAwaitStart(std::unique_ptr<AwaitStartLabel> lab)
	-> HandleResult<std::monostate>
{
	getExec().getGraph().getState().onAwaitStart(lab->getPos());
	return handleDummy(std::move(lab));
}

auto ExplorationCore::handleFaiZNESpinEnd(std::unique_ptr<FaiZNESpinEndLabel> lab)
	-> HandleResult<std::monostate>
{
	auto &g = getExec().getGraph();

	auto *zLab = genmc::dyn_cast<FaiZNESpinEndLabel>(addLabelToGraph(std::move(lab)));
	if (!inRandomMode() && spin_rules::faiZNEConstraintsSat(g, getConsChecker(), *zLab))
		blockThread(g, FaiZNEBlockLabel::create(zLab->getPos())); /* no moot desired */
	return {.result = std::monostate(), .count = 1U};
}

auto ExplorationCore::handleLockZNESpinEnd(std::unique_ptr<LockZNESpinEndLabel> lab)
	-> HandleResult<std::monostate>
{
	auto *zLab = addLabelToGraph(std::move(lab));
	blockThreadTryMoot(LockZNEBlockLabel::create(zLab->getPos()));
	return {.result = std::monostate(), .count = 1U};
}

auto ExplorationCore::handleDummy(std::unique_ptr<EventLabel> lab) -> HandleResult<std::monostate>
{
	addLabelToGraph(std::move(lab));
	return {.result = std::monostate(), .count = 1U};
}

auto ExplorationCore::handleLoopBegin(std::unique_ptr<LoopBeginLabel> lab)
	-> HandleResult<std::monostate>
{
	getExec().getGraph().getState().onLoopBegin(lab->getPos(), lab->getLoop());
	return handleDummy(std::move(lab));
}

/************************************************************
 ** Printing facilities
 ***********************************************************/

auto ExplorationCore::updateErrorInfoAndMaybeExit(Event pos, bool isNA, const EventDbgInfo *dbg)
	-> std::optional<VerificationError>
{
	/* If possible, update metadata */
	if (!getScheduler().inErrorReplay())
		return std::nullopt;
	if (dbg && (!isNA || Config::emitNALabels)) {
		errorReplay_->record(pos, *dbg);
	}

	/* Check whether error replay is complete */
	if (errorReplay_->reachedTarget(pos)) {
		auto details = *errorReplay_->buffered(); /* stop() erases it */
		auto code = details.type;
		auto halt = details.shouldHalt;
		reportError(pos, details);
		return halt ? std::optional(code) : std::nullopt;
	}
	return std::nullopt;
}

void ExplorationCore::inheritDbgInfo(Event from, const BlockLabel *bLab)
{
	auto &dbgInfo = errorReplay_->dbgInfo();
	if (auto it = dbgInfo.find(from); it != dbgInfo.end())
		dbgInfo.try_emplace(bLab->getPos(), it->second);
}

void ExplorationCore::configureProbe(MemLabel *rLab, Event pos, SAddr addr, ASize size,
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
