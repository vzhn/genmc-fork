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

#ifndef GENMC_GENMC_DRIVER_HPP
#define GENMC_GENMC_DRIVER_HPP

#include "genmc/Verification/Core/ExplorationCore.hpp"

#include <functional>
#include <unordered_set>

/**
 * The driver: the protocol between the exploration (ExplorationCore) and the
 * program that produces the events (see EventProducer). For each event the
 * producer reports, the driver works out whether the graph already has it
 * (the run replays the graph) or not (the core extends the graph), collects
 * debug information for error replays, and returns the value or thread the
 * producer needs. It also runs the checks at the start and end of each run.
 * The core does not call the driver: everything it needs is its own.
 */
class GenMCDriver : public ExplorationCore {

public:
	/** Called with every complete execution the exploration counts, and
	 * whether the memory model's whole-graph check accepts it (for tests) */
	using ExecutionHook = std::function<void(const ExecutionGraph &, bool consistent)>;
	void setExecutionHook(ExecutionHook hook) { executionHook_ = std::move(hook); }

	template <typename... Ts> static auto create(Ts &&...params) -> std::unique_ptr<GenMCDriver>
	{
		return std::unique_ptr<GenMCDriver>(new GenMCDriver(std::forward<Ts>(params)...));
	}

	/**** Generic actions ***/

	/** Returns to the interpreter the next thread to run (nullopt if none) */
	auto scheduleNext(std::span<Action> runnable) -> ScheduleResult;

	/** Attemps to complete the execution by inspecting the cache.
	 * Returns whether it succeeded. */
	auto runFromCache() -> bool;

	/** Should be called to register a static allocation, before the execution loop.
	 *  Returns an SAddr. For a thread-local variable, the address is the
	 *  main thread's copy; another thread's copy is the address with that
	 *  thread's id (SAddr::withThread()).
	 *
	 *  For EMIT_NA_LABELS=ON: @p initData must point to memory that will
	 *  hold the initial bytes. The pointer must remain valid for the driver's lifetime. */
#if EMIT_NA_LABELS
	auto allocateGlobal(ASize size, uint64_t alignment, const void *initData,
			    bool persistent = false, bool internal = false,
			    bool threadLocal = false) -> SAddr;
#else
	auto allocateGlobal(ASize size, uint64_t alignment, bool persistent = false,
			    bool internal = false, bool threadLocal = false) -> SAddr;
#endif

	/** Should be called at the beginning of each execution. Returns whether the frontend
	 * could collect debug information (non-mandatory) */
	auto handleExecutionStart() -> bool;

	/** Should be called at the end of each execution */
	auto handleExecutionEnd() -> std::optional<VerificationError>;

	/*** Instruction handling ***/

	/** A thread has just finished execution, nothing for the interpreter */
	auto handleThreadFinish(const EventDbgInfo *dbg, Event pos, SVal val,
				const EventDeps &deps = EventDeps())
		-> HandleResult<std::monostate>;

	/** A thread has terminated abnormally */
	auto handleThreadKill(const EventDbgInfo *dbg, Event pos) -> HandleResult<std::monostate>;

	/** This method blocks the current thread  */
	auto handleAssume(const EventDbgInfo *dbg, Event pos, AssumeType type)
		-> HandleResult<std::monostate>;

#if EMIT_NA_LABELS
#define GENMC_OLD_VAL_ARG
#define GENMC_NA_STORE_VAL_ARG SVal val,
#else
#define GENMC_NA_STORE_VAL_ARG
#define GENMC_OLD_VAL_ARG std::optional<SVal> oldVal,
#endif

/* Generation of Handler Declarations */
#define HANDLE_LOAD_LABEL(name)                                                                    \
	auto handle##name(const EventDbgInfo *dbg, Event pos, GENMC_OLD_VAL_ARG MemOrdering ord,   \
			  SAddr addr, ASize size, EventLabel *rfLab,                               \
			  const std::optional<Annotation> &annot, const EventDeps &deps)           \
		-> HandleResult<SVal>;

#define HANDLE_CAS_LOAD_LABEL(name)                                                                \
	auto handle##name(const EventDbgInfo *dbg, Event pos, GENMC_OLD_VAL_ARG MemOrdering ord,   \
			  MemOrdering failOrd, SAddr addr, ASize size, SVal exp, SVal swap,        \
			  WriteAttr wattr, bool weak, EventLabel *rfLab,                           \
			  const std::optional<Annotation> &annot, const EventDeps &deps)           \
		-> HandleResult<CasReadResult>;

#define HANDLE_LOCK_LOAD_LABEL(name)                                                               \
	auto handle##name(const EventDbgInfo *dbg, Event pos, GENMC_OLD_VAL_ARG SAddr addr,        \
			  ASize size, const std::optional<Annotation> &annot,                      \
			  const EventDeps &deps) -> HandleResult<SVal>;

#define HANDLE_FAI_LOAD_LABEL(name)                                                                \
	auto handle##name(const EventDbgInfo *dbg, Event pos, GENMC_OLD_VAL_ARG MemOrdering ord,   \
			  SAddr addr, ASize size, RMWBinOp op, SVal val, WriteAttr wattr,          \
			  EventLabel *rfLab, const std::optional<Annotation> &annot,               \
			  const EventDeps &deps) -> HandleResult<SVal>;

#define HANDLE_STORE_LABEL(name)                                                                   \
	auto handle##name(const EventDbgInfo *dbg, Event pos, GENMC_OLD_VAL_ARG MemOrdering ord,   \
			  SAddr addr, ASize size, SVal val, WriteAttr wattr,                       \
			  const EventDeps &deps) -> HandleResult<bool>;

#define HANDLE_LOCK_STORE_LABEL(name)                                                              \
	auto handle##name(const EventDbgInfo *dbg, Event pos, SAddr addr, ASize size,              \
			  const EventDeps &deps) -> HandleResult<bool>;

#define HANDLE_CAS_STORE_LABEL(name) HANDLE_STORE_LABEL(name)
#define HANDLE_FAI_STORE_LABEL(name) HANDLE_STORE_LABEL(name)

#include "genmc/Execution/EventLabel.def"

	auto handleNALoad(const EventDbgInfo *dbg, Event pos, SAddr loc, ASize size,
			  const EventDeps &deps) -> NALoadResult;
	auto handleNAStore(const EventDbgInfo *dbg, Event pos, SAddr loc, ASize size,
			   GENMC_NA_STORE_VAL_ARG const EventDeps &deps) -> NAStoreResult;

#undef GENMC_OLD_VAL_ARG
#undef GENMC_NA_STORE_VAL_ARG

	/** Returns an appropriate result for malloc() */
	auto handleMalloc(const EventDbgInfo *dbg, Event pos, ASize size, uint64_t alignment,
			  StorageDuration sdur, StorageType styp, AddressSpace spc,
			  const NameInfo *info, const std::string &name, const EventDeps &deps,
			  bool zeroed = false, bool threadPrivate = false) -> HandleResult<SVal>;

	/** A call to free() has been interpreted, nothing for the intepreter */
	auto handleFree(const EventDbgInfo *dbg, Event pos, SAddr loc, StorageDuration sdur,
			const EventDeps &deps) -> HandleResult<std::monostate>;
	auto handleRetire(const EventDbgInfo *dbg, Event pos, SAddr loc, const EventDeps &deps)
		-> HandleResult<std::monostate>;

	/** A fence has been interpreted, nothing for the interpreter */
	auto handleFence(const EventDbgInfo *dbg, Event pos, MemOrdering ord, const EventDeps &deps)
		-> HandleResult<std::monostate>;

	/** A Linux-kernel memory barrier (__VERIFIER_lkmm_fence()) has been interpreted */
	auto handleLkmmFence(const EventDbgInfo *dbg, Event pos, LkmmFenceKind kind,
			     const EventDeps &deps) -> HandleResult<std::monostate>;

	/** A synchronize_srcu() of the srcu_struct at SSP has been interpreted */
	auto handleSyncSrcu(const EventDbgInfo *dbg, Event pos, SAddr ssp, const EventDeps &deps)
		-> HandleResult<std::monostate>;

	/** Returns the TID of the newly created thread */
	auto handleThreadCreate(const EventDbgInfo *dbg, Event pos, const ThreadInfo &info,
				const EventDeps &deps) -> HandleResult<int>;

	/** Returns an appropriate result for pthread_join() */
	auto handleThreadJoin(const EventDbgInfo *dbg, Event pos, int childTid,
			      const EventDeps &deps, bool resultUsed = true) -> HandleResult<SVal>;

	/** A helping CAS operation has been interpreter.
	 * Returns whether the helped CAS is present. */
	auto handleHelpingCas(const EventDbgInfo *dbg, Event pos, MemOrdering ord, SAddr loc,
			      ASize size, SVal cmpVal, SVal newVal, const EventDeps &deps)
		-> HandleResult<std::monostate>;

	/** A call to __VERIFIER_opt_begin() has been interpreted.
	 * Returns whether the block should expand */
	auto handleOptional(const EventDbgInfo *dbg, Event pos) -> HandleResult<bool>;

	/** A call to __VERIFIER_spin_start() of LOOP has been interpreted */
	auto handleSpinStart(const EventDbgInfo *dbg, Event pos, LoopKey loop)
		-> HandleResult<std::monostate>;
	auto handleAwaitStart(const EventDbgInfo *dbg, Event pos) -> HandleResult<std::monostate>;

	/** A call to __VERIFIER_faiZNE_spin_end() has been interpreted */
	auto handleFaiZNESpinEnd(const EventDbgInfo *dbg, Event pos)
		-> HandleResult<std::monostate>;

	/** A call to __VERIFIER_lockZNE_spin_end() has been interpreted */
	auto handleLockZNESpinEnd(const EventDbgInfo *dbg, Event pos)
		-> HandleResult<std::monostate>;

	/** Helpers for dummy events */
	auto handleLoopBegin(const EventDbgInfo *dbg, Event pos, LoopKey loop)
		-> HandleResult<std::monostate>;
	auto handleHpProtect(const EventDbgInfo *dbg, Event pos, SAddr hpAddr, SAddr protAddr)
		-> HandleResult<std::monostate>;
	auto handleMethodBegin(const EventDbgInfo *dbg, Event pos, const std::string &methodName,
			       int32_t argVal) -> HandleResult<std::monostate>;
	auto handleMethodEnd(const EventDbgInfo *dbg, Event pos, const std::string &methodName,
			     int32_t retVal) -> HandleResult<std::monostate>;
	auto handleOutput(const EventDbgInfo *dbg, Event pos, std::string msg)
		-> HandleResult<std::monostate>;
	auto handleError(const EventDbgInfo *dbg, Event pos, std::string msg)
		-> HandleResult<std::monostate>;

	~GenMCDriver() override;

	GenMCDriver(const GenMCDriver &) = delete;
	auto operator=(const GenMCDriver &) -> GenMCDriver & = delete;
	GenMCDriver(GenMCDriver &&) = delete;
	auto operator=(GenMCDriver &&) -> GenMCDriver & = delete;

protected:
	friend class ThreadPool;
	friend class DriverHandlerDispatcher;
	friend void explore(GenMCDriver &driver, EventProducer &producer);

	GenMCDriver(std::shared_ptr<const Config> conf, WorkSharing *pool = nullptr,
		    Mode mode = VerificationMode{})
		: ExplorationCore(std::move(conf), pool, mode)
	{}

	/* The core's handlers for labels, next to the producer-facing ones */
	using ExplorationCore::handleAwaitStart;
	using ExplorationCore::handleFaiZNESpinEnd;
	using ExplorationCore::handleFence;
	using ExplorationCore::handleFree;
	using ExplorationCore::handleHelpingCas;
	using ExplorationCore::handleLockZNESpinEnd;
	using ExplorationCore::handleLoopBegin;
	using ExplorationCore::handleMalloc;
	using ExplorationCore::handleNALoad;
	using ExplorationCore::handleNAStore;
	using ExplorationCore::handleOptional;
	using ExplorationCore::handleRetire;
	using ExplorationCore::handleSpinStart;
	using ExplorationCore::handleThreadCreate;
	using ExplorationCore::handleThreadFinish;
	using ExplorationCore::handleThreadJoin;
	using ExplorationCore::handleThreadKill;

	/** Wraps the value ready by a CAS into a CasReadResult, computing
	 * the success flag from the read label at @p pos. Non-value results
	 * (errors, resets) are propagated unchanged. */
	auto toCasResult(HandleResult<SVal> res, Event pos) -> HandleResult<CasReadResult>;

private:
	/** See setExecutionHook() */
	ExecutionHook executionHook_;

	/** Liveness: Reports an error on liveness violations */
	auto checkLiveness() -> std::optional<VerificationError>;

	/** Reports an error if there is unfreed memory */
	auto checkUnfreedMemory() -> std::optional<VerificationError>;

	/** Under LKMM, reports a data race of the complete execution */
	auto checkLkmmRace() -> std::optional<VerificationError>;

	/** -exhaustive under IMM: a data race in the execution, once its
	 * threads have reached their kept events */
	auto checkExhaustiveRace() -> std::optional<VerificationError>;

	/** Under LKMM, checks a complete execution for a misuse of SRCU (the
	 * kernel model's flags) */
	auto checkLkmmSrcu() -> std::optional<VerificationError>;

	/** Adds each one of LABS to graph (maintains well-formedness) */
	void addLabelsToGraph(const std::vector<std::unique_ptr<EventLabel>> &labs);

	/** Helper: Checks whether the user annotation about helped/helping CASes seems OK */
	void checkHelpingCasAnnotation();

	/** Makes an estimation about the state space and updates the current one.
	 * Has to run at the end of an execution */
	void updateStSpaceEstimation();

	auto fullExecutionExceedsBound() const -> bool;

	/** Update bounds histogram with the current, complete execution */
	void trackExecutionBound();

	auto getDbgInfo(Event pos) -> const EventDbgInfo *;

	/** -exhaustive: the canonical hashes of the complete executions counted */
	std::unordered_set<GraphHash> countedExecutions_;
};

#endif /* GENMC_GENMC_DRIVER_HPP */
