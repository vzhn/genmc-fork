// For the parts of the code originating from LLVM-3.5:
//===-- Interpreter.h ------------------------------------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LLVMLICENSE for details.
//
//===----------------------------------------------------------------------===//
//
// This header file defines the interpreter structure
//
//===----------------------------------------------------------------------===//

/*
 * (For the parts of the code modified from LLVM-3.5)
 *
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

#ifndef LLI_INTERPRETER_H
#define LLI_INTERPRETER_H

#include "Runtime/DepTracker.hpp"
#include "genmc/ADT/View.hpp"
#include "genmc/ADT/value_ptr.hpp"
#include "genmc/Execution/LoadAnnotation.hpp"
#include "genmc/Support/ActionEnums.hpp"
#include "genmc/Support/MemAccess.hpp"
#include "genmc/Support/SAddr.hpp"
#include "genmc/Support/SVal.hpp"
#include "genmc/Support/ThreadInfo.hpp"
#include "genmc/Verification/GenMCDriver.hpp"
#include "genmc/Verification/ResumePoint.hpp"
#include "genmc/Verification/VerificationError.hpp"
#include "passes/InternalFunctions.hpp"
#include "passes/LLVMUtils.hpp"
#include "passes/ModuleInfo.hpp"

#include <functional>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/IntervalMap.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/DataTypes.h>
#include <llvm/Support/ErrorHandling.h>

#include <filesystem>
#include <format>
#include <optional>
#include <random>
#include <unordered_map>
#include <unordered_set>

#if LLVM_VERSION_MAJOR >= 20
#define APINT_NOCHECK , true
#else
#define APINT_NOCHECK
#endif

/* Some helpers for GenericValues */
#define INT_TO_GV(typ, val)                                                                        \
	({                                                                                         \
		llvm::GenericValue __ret;                                                          \
		__ret.IntVal =                                                                     \
			llvm::APInt((typ)->getIntegerBitWidth(), (val), true APINT_NOCHECK);       \
		__ret;                                                                             \
	})

#define PTR_TO_GV(ptr)                                                                             \
	({                                                                                         \
		llvm::GenericValue __ret;                                                          \
		__ret.PointerVal = (void *)(ptr);                                                  \
		__ret;                                                                             \
	})

#define GET_ZERO_GV(typ)                                                                           \
	({                                                                                         \
		llvm::GenericValue __ret;                                                          \
		if (typ->isPointerTy())                                                            \
			__ret = PTR_TO_GV(nullptr);                                                \
		else                                                                               \
			__ret = INT_TO_GV(typ, 0);                                                 \
		__ret;                                                                             \
	})

class GenMCDriver;
struct LLIConfig;

namespace llvm {

class IntrinsicLowering;
struct FunctionInfo;
template <typename T> class generic_gep_type_iterator;
class ConstantExpr;
typedef generic_gep_type_iterator<User::const_op_iterator> gep_type_iterator;

typedef std::vector<GenericValue> ValuePlaneTy;

// AllocaHolder - Object to track all of the blocks of memory allocated by
// allocas in a particular stack frame. Since the driver needs to be made
// aware of the deallocs, special care needs to be taken to inform the driver
// when stack frames are popped
//
class AllocaHolder {
	std::vector<void *> allocas;

public:
	using Allocas = std::vector<void *>;

	AllocaHolder() {}

	void add(void *mem) { allocas.push_back(mem); }
	const Allocas &get() const { return allocas; }
};

// ExecutionContext struct - This struct represents one stack frame currently
// executing.
//
// What the code of a branch (all its paths, up to the join) can order before
// the events after its join in GenMC's prefix (the kat's ppo): see
// Interpreter::updateBranchCtrlDeps()
//
// Only the branch's writes (and fences) matter: they carry the branch's
// condition in their prefix (ctrl ; [W]), and a revisit that keeps an event
// after the join is blocked on the path that puts one into its prefix but
// not on the other, while the branch's read is deleted by it there
struct BranchRegion {
	bool ordersAll = false;	   // a fence, a call that may order...
	bool anyLoc = false;	   // a write whose address is not known at the branch
	SmallPtrSet<Value *, 4> ptrs; // the addresses of the writes (known at the branch)
	bool markedWrites = false; // marked writes, for wmb, mb, po-rel

	bool hasWrites() const { return anyLoc || !ptrs.empty(); }
	void merge(const BranchRegion &o)
	{
		ordersAll |= o.ordersAll;
		anyLoc |= o.anyLoc;
		ptrs.insert(o.ptrs.begin(), o.ptrs.end());
		markedWrites |= o.markedWrites;
	}
};

// A branch's dependencies, the addresses its code may write and what it
// can order (see BranchRegion)
struct CtrlShadow {
	DepInfo deps;
	bool anyLoc = false;
	bool markedWrites = false;
	SmallVector<void *, 4> addrs;

	void merge(const CtrlShadow &o)
	{
		deps.update(o.deps);
		anyLoc |= o.anyLoc;
		markedWrites |= o.markedWrites;
		addrs.append(o.addrs.begin(), o.addrs.end());
	}
};

// An open branch: its control dependencies hold until its join
struct CtrlScope {
	const BasicBlock *join;
	bool ordersAll;
	CtrlShadow shadow;
};

/* The values of a frame. A copy shares the map with its original until one
 * of them writes to it: the resume points (see Interpreter::saveResumePoint())
 * copy whole threads, and a frame's values rarely change afterwards, except
 * the top frame's */
class FrameValues {
	using Map = llvm::DenseMap<Value *, GenericValue>;

public:
	auto operator[](Value *v) -> GenericValue &
	{
		own();
		return (*map_)[v];
	}
	[[nodiscard]] auto lookup(Value *v) const -> GenericValue
	{
		return map_ ? map_->lookup(v) : GenericValue();
	}
	[[nodiscard]] auto find(Value *v) const -> Map::const_iterator
	{
		return map_ ? map_->find(v) : end();
	}
	[[nodiscard]] auto end() const -> Map::const_iterator
	{
		return map_ ? map_->end() : emptyMap().end();
	}

private:
	void own()
	{
		if (!map_)
			map_ = std::make_shared<Map>();
		else if (map_.use_count() > 1)
			map_ = std::make_shared<Map>(*map_);
	}

	static auto emptyMap() -> const Map &
	{
		static const Map empty;
		return empty;
	}

	std::shared_ptr<Map> map_;
};

struct ExecutionContext {
	Function *CurFunction;			      // The currently executing function
	BasicBlock *CurBB;			      // The currently executing BB
	BasicBlock::iterator CurInst;		      // The next instruction to execute
	CallInstWrapper Caller;			      // Holds the call that called subframes.
						      // NULL if main func or debugger invoked fn
	FrameValues Values;			      // LLVM values used in this invocation
	std::vector<GenericValue> VarArgs;	      // Values passed through an ellipsis
	AllocaHolder Allocas;			      // Track memory allocated by alloca

	// With -check-replay-keys (see EventKey): how many times each
	// instruction of this frame has run (calls, and instructions that
	// call the driver), and the caller's call instance that made the frame
	llvm::DenseMap<const Instruction *, std::uint32_t> KeyCounts;
	EventKey::Step KeyCall{0, 0};

	// Under LKMM (scopesCtrlDeps()): the control dependencies of the open
	// branches of this frame, each until its join (the branch's immediate
	// post-dominator) is entered, and those of the whole frame (an indirect
	// call's function pointer)
	llvm::SmallVector<CtrlScope, 2> CtrlScopes;
	DepInfo FrameCtrl;

	ExecutionContext() : CurFunction(nullptr), CurBB(nullptr), CurInst(nullptr) {}
};

/* The different reasons a thread might block */
enum class BlockageType {
	NotBlocked,
	ThreadJoin,
	Spinloop,
	FaiZNESpinloop,
	LockZNESpinloop,
	HelpedCas,
	Confirmation,
	ReadOptBlock,
	Barrier,
	Cons,
	Error,
	User,
};

/*
 * Thread class -- Contains information specific to each thread.
 */
class Thread {

public:
	using MyRNG = std::minstd_rand;
	using MyDist = std::uniform_int_distribution<MyRNG::result_type>;
	static constexpr int seed = 1995;

	Thread() = default;

	int id{};
	int parentId{};
	llvm::Function *threadFun{};
	SVal threadArg;
	std::vector<llvm::ExecutionContext> ECStack;
	std::vector<llvm::ExecutionContext> initEC;
	BlockageType blocked{};
	MyRNG rng;
	GenMCDriver::EventDbgInfo currDbgInfo;

	/* With -check-replay-keys: the instruction instance whose events are
	 * being added (see Interpreter::prepareEventKey()) */
	std::uint64_t visitNo = 0;
	std::optional<std::uint64_t> keyVisit;
	const llvm::Instruction *keyInst{};
	std::size_t keyDepth{};
	std::shared_ptr<const EventKey> key;
	Event keyBase;

	/* The thread's control dependencies, with those of the open branches */
	DepInfo ctrlScratch;

	/* The prefix-only ones of the current event (shadows, see EventDeps) */
	DepInfo pctrlScratch;

	/* Branches joined whose code accessed memory: the writes after the join
	 * to a location the branch may access get their dependencies (moi, fri),
	 * and so does all after an ordering event (fences, releases...) */
	std::vector<llvm::CtrlShadow> ctrlShadows;

	/* A copy of this thread's state shared by the resume points saved
	 * since it last ran (see Interpreter::saveResumePoint()); dropped
	 * whenever the thread is about to change */
	mutable std::shared_ptr<const Thread> frozen;
	void thaw() { frozen.reset(); }

	bool isMain() const { return id == 0; }

	void block(BlockageType t)
	{
		blocked = t;
		thaw();
	}
	void unblock()
	{
		blocked = BlockageType::NotBlocked;
		thaw();
	}
	bool isBlocked() const { return blocked != BlockageType::NotBlocked; }
	BlockageType getBlockageType() const { return blocked; }

protected:
	friend class Interpreter;

	Thread(llvm::Function *F, int id)
		: id(id), parentId(-1), threadFun(F), initEC(), blocked(BlockageType::NotBlocked),
		  rng(seed)
	{}

	Thread(llvm::Function *F, SVal arg, int id, int pid, const llvm::ExecutionContext &SF)
		: id(id), parentId(pid), threadFun(F), threadArg(arg), initEC({SF}),
		  blocked(BlockageType::NotBlocked), rng(seed)
	{}
};

/* The state of the program -- i.e., part of the program being interpreted */
enum class ProgramState { Ctors, Main, Dtors };

/* The state of the current execution */
enum class ExecutionState { Normal, Replay };

struct DynamicComponents {

	DynamicComponents() : nameInfo(imapAllocator) {}

	/* Information about threads as well as the currently executing thread */
	std::vector<Thread> threads;
	std::vector<Action> globalInstructions;
	int currentThread = 0;

	/* Pointer to the dependency tracker */
	value_ptr<DepTracker, DepTrackerCloner> depTracker = nullptr;

	/* Information about the interpreter's state */
	ProgramState programState = ProgramState::Main; /* Pers */

	/* Whether we should collect dbg info + naming info during error runs */
	using AddrIntervalMap = IntervalMap<SAddr, std::pair<std::string, const NameInfo *>, 16,
					    IntervalMapHalfOpenInfo<SAddr>>;
	bool collectDbgInfo = false;
	AddrIntervalMap::Allocator imapAllocator;
	AddrIntervalMap nameInfo;

	GenericValue ExitValue; // The return value of the called function

	// AtExitHandlers - List of functions to call when the program exits,
	// registered with the atexit() library function.
	std::vector<Function *> AtExitHandlers;
};

using InterpreterState = DynamicComponents;

// Interpreter - This class represents the entirety of the interpreter.
//
class Interpreter : public ExecutionEngine, public InstVisitor<Interpreter> {

protected:
	using AnnotID = ModuleID::ID;
	using AnnotT = SExpr<AnnotID>;

	/*** Static components (once set, do not change) ***/

	/* Information about the module under test */
	std::unique_ptr<ModuleInfo> MI;

	/* In parallel mode every worker interprets its own clone of the module,
	 * but workers exchange graphs, and so the function pointers stored in
	 * memory. A function's address is therefore the Function* of the module
	 * all clones were made from (it outlives the workers). These map this
	 * interpreter's functions to those addresses and back. Both are empty
	 * when the interpreter runs on the original module. */
	llvm::DenseMap<const Function *, void *> funToCanon_;
	llvm::DenseMap<const void *, Function *> canonToFun_;

	/* With -check-replay-keys: each instruction's number in module order,
	 * the same in every worker's copy of the module (see EventKey) */
	llvm::DenseMap<const Instruction *, std::uint32_t> keyInstIds_;

	/* Summaries of branch regions (by branch block) and of functions */
	llvm::DenseMap<const BasicBlock *, BranchRegion> branchRegions_;
	llvm::DenseMap<const Function *, std::optional<BranchRegion>> funRegions_;

	/* Post-dominator trees, per function (see getBranchJoin()) */
	llvm::DenseMap<const Function *, std::unique_ptr<llvm::PostDominatorTree>> postDoms_;

	/* Whether an alloca's address never leaves its function (see
	 * isThreadPrivateAlloca()), per alloca */
	llvm::DenseMap<const AllocaInst *, bool> privateAllocas_;

	/* Registers (and their Values) mentioned by each load annotation; filled
	 * lazily by getCurrentAnnotConcretized() */
	std::unordered_map<AnnotID, std::vector<std::pair<AnnotID, const Value *>>> annotRegs_;

	/* Maps internal functions to their code. Derived once from internalFunNames */
	llvm::DenseMap<const llvm::Function *, InternalFunctions> internalFunCache_;

	/* The directories of GenMC's runtime headers, and whether each file
	 * met in debug locations is one of them (see getReportedLoc()) */
	std::vector<std::filesystem::path> runtimeIncludeDirs_;
	llvm::DenseMap<const llvm::DIFile *, bool> runtimeHeaders_;

	/* The address of the main thread's copy of each thread-local variable
	 * (another thread's copy has that thread's id: see SAddr) */
	llvm::DenseMap<const GlobalVariable *, SAddr> threadLocalAddrs_;

	/* Mapping between static allocation beginning and the actual addresses of
	 * global variables (where their contents are stored) */
	std::unordered_map<SAddr, void *> staticValueMap;

	/* Keep all static ranges that have been allocated */
	VSet<std::pair<SAddr, SAddr>> staticAllocas;

	/* Maintain the relationship between SAddr and global variables,
	 * so that we can get naming information */
	std::unordered_map<SAddr, Value *> staticNames;

	/* (Composition) pointer to the driver */
	GenMCDriver *driver;

	Function *mainFun = nullptr;

	/* Whether the driver should be called on system errors */
	bool stopOnSystemErrors;

	/* Where system errors return values should be stored (if required) */
	SAddr errnoAddr;
	Type *errnoTyp;

	/* This is not exactly static but is reset to the same value each time*/
	std::vector<ExecutionContext> mainECStack;

	IntrinsicLowering *IL;

	/*** Dynamic components (change during verification) ***/

	DynamicComponents dynState;

	/* Whether an instruction is in a call to the driver (see CALL_DRIVER),
	 * and the current thread's event position before the instruction (an
	 * instruction may add two events, with a driver call each) */
	bool callingDriver_ = false;
	Event visitStartPos_;

public:
	explicit Interpreter(std::unique_ptr<Module> M, std::unique_ptr<ModuleInfo> MI,
			     GenMCDriver *driver, const LLIConfig *userConf,
			     const Module *canonicalMod = nullptr);
	virtual ~Interpreter();

	/* Saves the dynamic state at the event at POS, which the current thread
	 * is adding (see ResumePoint); null if the runtime is not inside the
	 * instruction that adds it */
	auto saveResumePoint(Event pos) -> std::shared_ptr<ResumePoint>;

	/* Restores a saved state and runs on from it, instead of runMain(),
	 * unless a thread of the state is past the graph (an event of its that
	 * the restriction removed had been re-added after the saved event, see
	 * GenMCDriver::saveResumePoint()): then nothing is restored and the
	 * graph has to be replayed. TIS describes the graph's threads (the
	 * restriction may have kept the begin of a thread the state does not
	 * have, which is added as new) and LASTIDX each thread's last event */
	auto resumeFrom(const ResumePoint &rp, const std::vector<ThreadInfo> &tis,
			const std::vector<int> &lastIdx) -> bool;

	Thread &constructAddThreadFromInfo(const ThreadInfo &ti)
	{
		auto *calledFun =
			dyn_cast<Function>(const_cast<Value *>(MI->idInfo.IDV.at(ti.funId)));
		VERIFY(calledFun);
		ExecutionContext SF;

		SF.CurFunction = calledFun;
		SF.CurBB = &calledFun->front();
		SF.CurInst = SF.CurBB->begin();

		SF.Values[&*calledFun->arg_begin()] = PTR_TO_GV(ti.arg.get());
		return createAddNewThread(calledFun, ti.arg, ti.id, ti.parentId, SF);
	}
	void setExecutionContext(const std::vector<ThreadInfo> &tis)
	{
		dynState.threads.clear();
		dynState.globalInstructions.clear();
		createAddMainThread();
		for (auto &ti : tis)
			constructAddThreadFromInfo(ti);
	}

#define CALL_DRIVER(method, ...)                                                                   \
	({                                                                                         \
		if (checkReplayKeys())                                                             \
			prepareEventKey();                                                         \
		callingDriver_ = true;                                                             \
		auto res = driver->method(__VA_ARGS__);                                            \
		callingDriver_ = false;                                                            \
		if (checkReplayKeys())                                                             \
			driver->clearEventKey();                                                   \
		if (std::holds_alternative<GenMCDriver::Reset>(res.result)) {                      \
			--ECStack().back().CurInst;                                                \
			if (checkReplayKeys())                                                     \
				undoEventKey();                                                    \
		}                                                                                  \
		incPos(res.count);                                                                 \
		res.result;                                                                        \
	})

	/* Whether events get keys and replay checks them (see EventKey) */
	bool checkReplayKeys() const { return driver->checksReplayKeys(); }

	/* An instruction's number in keys (0 for none) */
	std::uint32_t keyInstId(const Instruction *I) const
	{
		return I ? keyInstIds_.lookup(I) : 0;
	}

	/* Tells the driver which instruction instance the current thread is
	 * about to call it for: a new one unless the same visit of the same
	 * instruction already did (e.g. a CAS's read, then its write) */
	void prepareEventKey();

	/* The instruction will be visited again (the driver reset it): it
	 * does not count as a run */
	void undoEventKey();

	/* Blocks the current execution */
	void block(BlockageType t = BlockageType::Error)
	{
		std::for_each(threads_begin(), threads_end(), [&](Thread &thr) { thr.block(t); });
	}

	/* Resets all thread info to their initial values */
	void resetThread(unsigned int id);

	/* Resets the interpreter at the beginning of a new execution */
	void reset();

	/* Creates a new thread and adds it to the thread list */
	Thread &createAddNewThread(llvm::Function *F, SVal arg, int tid, int pid,
				   const llvm::ExecutionContext &SF);

	/* Returns the currently executing thread */
	Thread &getCurThr() { return dynState.threads[dynState.currentThread]; }
	const Thread &getCurThr() const { return dynState.threads.at(dynState.currentThread); }

	/* Returns the thread with the specified ID (taken from the graph) */
	Thread &getThrById(int id) { return dynState.threads[id]; };
	const Thread &getThrById(int id) const { return dynState.threads[id]; };

	unsigned int getNumThreads() const { return dynState.threads.size(); }

	using thread_iterator = std::vector<Thread>::iterator;
	using const_thread_iterator = std::vector<Thread>::const_iterator;
	using thread_range = iterator_range<thread_iterator>;
	using const_thread_range = iterator_range<const_thread_iterator>;

	const_thread_iterator threads_begin() const { return dynState.threads.begin(); }
	const_thread_iterator threads_end() const { return dynState.threads.end(); }
	thread_iterator threads_begin() { return dynState.threads.begin(); }
	thread_iterator threads_end() { return dynState.threads.end(); }
	const_thread_range threads() const
	{
		return const_thread_range(threads_begin(), threads_end());
	}
	thread_range threads() { return thread_range(threads_begin(), threads_end()); }

	/* Returns the stack frame of the currently executing thread */
	std::vector<ExecutionContext> &ECStack() { return getCurThr().ECStack; }

	/* Returns the current (global) position (thread, index) interpreted */
	Event currPos() const { return dynState.globalInstructions[getCurThr().id].event; }
	Event nextPos() const { return currPos().next(); }
	Event incPos(int n)
	{
		auto &pos = dynState.globalInstructions[getCurThr().id].event;
		pos.index += n;
		return pos;
	}
	Event decPos() { return --dynState.globalInstructions[getCurThr().id].event; }

	/* Returns debug metadata for the current instruction. Returns nullptr if
	 * we're not in debug collection mode (see runAsMain()), or if no metadata exist.*/
	const GenMCDriver::EventDbgInfo *currDbgInfo(std::optional<SAddr> loc = {});
	std::string getAccessedVarName(const std::optional<SAddr> &loc);

	Event threadPos(unsigned int i) const { return dynState.globalInstructions[i].event; }

	/* Query interpreter's state */
	ProgramState getProgramState() const { return dynState.programState; }

	/* Annotation information */

	/* Returns (concretized) annotation information for the
	 * current instruction (assuming we're executing it) */
	std::optional<Annotation> getCurrentAnnotConcretized();

	/* Memory pools checks */

	/* Returns true if the interpreter has allocated space for the specified static */
	bool isStaticallyAllocated(SAddr addr) const;
	void *getStaticAddr(SAddr addr) const;
	std::string getStaticName(SAddr addr) const;

	/* Returns the executing thread's copy of the thread-local GV */
	SAddr getThreadLocalAddr(const GlobalVariable *GV) const;

	/// runAtExitHandlers - Run any functions registered by the program's calls to
	/// atexit(3), which we intercept and store in AtExitHandlers.
	///
	void runAtExitHandlers();

	/// create - Create an interpreter ExecutionEngine. This can never fail.
	///
	static std::unique_ptr<Interpreter> create(std::unique_ptr<Module> M,
						   std::unique_ptr<ModuleInfo> MI,
						   GenMCDriver *driver, const LLIConfig *userConf,
						   std::string *ErrorStr = nullptr,
						   const Module *canonicalMod = nullptr);

/// run - Start execution with the specified function and arguments.
///
#ifdef LLVM_EXECUTION_ENGINE_RUN_FUNCTION_VECTOR
	virtual GenericValue runFunction(Function *F, const std::vector<GenericValue> &ArgValues);
#else
	virtual GenericValue runFunction(Function *F, llvm::ArrayRef<GenericValue> ArgValues);
#endif

	void *getPointerToNamedFunction(const std::string &Name, bool AbortOnFailure = true)
	{
		// FIXME: not implemented.
		return nullptr;
	}

	void *getPointerToNamedFunction(llvm::StringRef Name, bool AbortOnFailure = true)
	{
		// FIXME: not implemented.
		return nullptr;
	};

	/// recompileAndRelinkFunction - For the interpreter, functions are always
	/// up-to-date.
	///
	void *recompileAndRelinkFunction(Function *F) { return getPointerToFunction(F); }

	/// freeMachineCodeForFunction - The interpreter does not generate any code.
	///
	void freeMachineCodeForFunction(Function *F) {}

	/* Helper functions */

	unsigned int getTypeSize(Type *typ) const;

	// Methods used to execute code:
	// Place a call on the stack
	void callFunction(Function *F, const std::vector<GenericValue> &ArgVals,
			  const std::unique_ptr<EventDeps> &specialDeps);

	/* callFunction() wrappers to be called before running a function */
	void setupFunctionCall(Function *F, ArrayRef<GenericValue> ArgValues);
	void setupStaticCtorsDtors(Module &M, bool isDtors);
	void setupStaticCtorsDtors(bool isDtors);
	void setupMain(Function *Fn, const std::vector<std::string> &argv, const char *const *envp);

	void run(); // Execute instructions until nothing left to do

	/* run() wrappers */
	int runMain(bool collectDbg = false);

	// Opcode Implementations
	void visitReturnInst(ReturnInst &I);
	void visitBranchInst(BranchInst &I);
	void visitSwitchInst(SwitchInst &I);
	void visitIndirectBrInst(IndirectBrInst &I);

	void visitBinaryOperator(BinaryOperator &I);
	void visitICmpInst(ICmpInst &I);
	void visitFCmpInst(FCmpInst &I);
	void visitAllocaInst(AllocaInst &I);
	void visitLoadInst(LoadInst &I);
	void visitStoreInst(StoreInst &I);
	void visitGetElementPtrInst(GetElementPtrInst &I);
	void visitPHINode(PHINode &PN) { llvm_unreachable("PHI nodes already handled!"); }
	void visitTruncInst(TruncInst &I);
	void visitZExtInst(ZExtInst &I);
	void visitSExtInst(SExtInst &I);
	void visitFPTruncInst(FPTruncInst &I);
	void visitFPExtInst(FPExtInst &I);
	void visitUIToFPInst(UIToFPInst &I);
	void visitSIToFPInst(SIToFPInst &I);
	void visitFPToUIInst(FPToUIInst &I);
	void visitFPToSIInst(FPToSIInst &I);
	void visitPtrToIntInst(PtrToIntInst &I);
	void visitIntToPtrInst(IntToPtrInst &I);
	void visitBitCastInst(BitCastInst &I);
	void visitFreezeInst(FreezeInst &I);
	void visitSelectInst(SelectInst &I);

	void visitCallInstWrapper(CallInstWrapper CIW);
	void visitCallBase(CallBase &CB) { visitCallInstWrapper(CallInstWrapper(CB)); }
	void visitUnreachableInst(UnreachableInst &I);

	void visitShl(BinaryOperator &I);
	void visitLShr(BinaryOperator &I);
	void visitAShr(BinaryOperator &I);

	void visitVAArgInst(VAArgInst &I);
	void visitExtractElementInst(ExtractElementInst &I);
	void visitInsertElementInst(InsertElementInst &I);
	void visitShuffleVectorInst(ShuffleVectorInst &I);

	void visitExtractValueInst(ExtractValueInst &I);
	void visitInsertValueInst(InsertValueInst &I);

	void visitAtomicCmpXchgInst(AtomicCmpXchgInst &I);
	void visitAtomicRMWInst(AtomicRMWInst &I);
	void visitFenceInst(FenceInst &I);

	bool isInlineAsm(CallInstWrapper CIW, std::string *asmStr);
	void visitInlineAsm(CallInstWrapper CIW, const std::string &asmString);

	void visitInstruction(Instruction &I)
	{
		errs() << I << "\n";
		llvm_unreachable("Instruction not interpretable yet!");
	}

	GenericValue callExternalFunction(Function *F, const std::vector<GenericValue> &ArgVals);
	void exitCalled(GenericValue GV);

	void addAtExitHandler(Function *F) { dynState.AtExitHandlers.push_back(F); }

	/* Inverse of getPointerToFunction(): the local function a pointer value
	 * denotes. Every place that calls through a pointer goes through here. */
	auto getFunctionFromPointer(void *p) const -> Function *
	{
		auto it = canonToFun_.find(p);
		return it == canonToFun_.end() ? static_cast<Function *>(p) : it->second;
	}

	GenericValue *getFirstVarArg() { return &(ECStack().back().VarArgs[0]); }

private: // Helper functions
	GenericValue executeGEPOperation(Value *Ptr, gep_type_iterator I, gep_type_iterator E,
					 ExecutionContext &SF);

	// SwitchToNewBasicBlock - Start execution in a new basic block and run any
	// PHI nodes in the top of the block.  This is used for intraprocedural
	// control flow.
	//
	void SwitchToNewBasicBlock(BasicBlock *Dest, ExecutionContext &SF);

	/* A function's address is the Function* of the canonical module (see
	 * funToCanon_), so that all workers of a thread pool agree on it */
	void *getPointerToFunction(Function *F)
	{
		auto it = funToCanon_.find(F);
		return it == funToCanon_.end() ? (void *)F : it->second;
	}
	void *getPointerToBasicBlock(BasicBlock *BB) { return (void *)BB; }

	void initializeExecutionEngine() {}
	void initializeExternalFunctions();
	GenericValue getConstantExprValue(ConstantExpr *CE, ExecutionContext &SF);
	GenericValue getOperandValue(Value *V, ExecutionContext &SF);
	GenericValue executeTruncInst(Value *SrcVal, Type *DstTy, ExecutionContext &SF);
	GenericValue executeSExtInst(Value *SrcVal, Type *DstTy, ExecutionContext &SF);
	GenericValue executeZExtInst(Value *SrcVal, Type *DstTy, ExecutionContext &SF);
	GenericValue executeFPTruncInst(Value *SrcVal, Type *DstTy, ExecutionContext &SF);
	GenericValue executeFPExtInst(Value *SrcVal, Type *DstTy, ExecutionContext &SF);
	GenericValue executeFPToUIInst(Value *SrcVal, Type *DstTy, ExecutionContext &SF);
	GenericValue executeFPToSIInst(Value *SrcVal, Type *DstTy, ExecutionContext &SF);
	GenericValue executeUIToFPInst(Value *SrcVal, Type *DstTy, ExecutionContext &SF);
	GenericValue executeSIToFPInst(Value *SrcVal, Type *DstTy, ExecutionContext &SF);
	GenericValue executePtrToIntInst(Value *SrcVal, Type *DstTy, ExecutionContext &SF);
	GenericValue executeIntToPtrInst(Value *SrcVal, Type *DstTy, ExecutionContext &SF);
	GenericValue executeBitCastInst(Value *SrcVal, Type *DstTy, ExecutionContext &SF);
	GenericValue executeCastOperation(Instruction::CastOps opcode, Value *SrcVal, Type *Ty,
					  ExecutionContext &SF);
	std::vector<GenericValue>
	translateExternalCallArgs(Function *F, const std::vector<GenericValue> &Args) const;
	void returnValueToCaller(Type *RetTy, GenericValue Result);
	void popStackAndReturnValueToCaller(Type *RetTy, GenericValue Result,
					    ReturnInst *retI = nullptr, DepInfo exitDeps = DepInfo());

	void setProgramState(ProgramState s) { dynState.programState = s; }

	void handleLock(SAddr addr, ASize size, const EventDeps *deps);
	void handleUnlock(SAddr addr, ASize size, const EventDeps *deps);

	/* Custom Opcode Implementations */

#define HANDLE_FUNCTION(NUM, FUN, NAME)                                                            \
	void call##NAME(Function *F, const std::vector<GenericValue> &ArgVals,                     \
			const std::unique_ptr<EventDeps> &specialDeps);
#include "passes/InternalFunction.def"

	/* Returns whether F is an internal function */
	auto isInternalCall(const Function *F) const -> bool;

	void callInternalFunction(Function *F, const std::vector<GenericValue> &ArgVals,
				  const std::unique_ptr<EventDeps> &deps);

	void freeAllocas(const AllocaHolder &allocas);

	/* Collects the addresses (and some naming information) for all variables with
	 * static storage. Also calculates the starting address of the allocation pool */
	void collectStaticAddresses();

	/* Sets up how some errors will be reported to the user */
	void setupErrorPolicy(Module *M, const LLIConfig *userConf);

	/* The location THR's current instruction is reported at: outside
	 * GenMC's runtime headers if possible (see isRuntimeHeader()) */
	auto getReportedLoc(Thread &thr) -> const llvm::DILocation *;

	/* Whether LOC is in one of GenMC's runtime headers (e.g., <pthread.h>) */
	auto isRuntimeHeader(const llvm::DILocation &loc) -> bool;

	/* Sets-up the specified thread for execution */
	void scheduleThread(int tid)
	{
		dynState.currentThread = tid;
		dynState.threads[tid].thaw();
	}

	/* Adds the specified thread to the list */
	Thread &addNewThread(Thread &&thread);

	/* Creates an entry for the main() function. More information are
	 * filled from the execution engine when the exploration starts */
	Thread &createAddMainThread();

	/* Dependency tracking */

	DepTracker *getDepTracker() { return &*dynState.depTracker; }
	const DepTracker *getDepTracker() const { return &*dynState.depTracker; }

	std::unique_ptr<EventDeps> makeEventDeps(const DepInfo *addr, const DepInfo *data,
						 const DepInfo *ctrl, const DepInfo *addrPo,
						 const DepInfo *cas);

	const DepInfo *getDataDeps(unsigned int tid, Value *i)
	{
		return getDepTracker() ? getDepTracker()->getDataDeps(tid, i) : nullptr;
	}
	const DepInfo *getAddrPoDeps(unsigned int tid)
	{
		return getDepTracker() ? getDepTracker()->getAddrPoDeps(tid) : nullptr;
	}
	const DepInfo *getCtrlDeps(unsigned int tid);

	/* Whether control dependencies end at the join of their branch */
	bool scopesCtrlDeps() const { return driver->scopesCtrlDeps(); }

	/* A conditional branch (or switch) at the end of BB on COND */
	void updateBranchCtrlDeps(BasicBlock *BB, Value *Cond);

	/* After the read of the CAS I (see the definition) */
	void updateCasCtrlDeps(AtomicCmpXchgInst &I);

	/* Whether the address of AI's memory never leaves AI's function, so only
	 * the thread running the function can access the memory */
	bool isThreadPrivateAlloca(const AllocaInst *AI);

	/* The join of the branch ending BB (nullptr: the function's exit) */
	const BasicBlock *getBranchJoin(BasicBlock *BB);

	/* What the code between the branch ending BB and JOIN can order */
	const BranchRegion &getBranchRegion(BasicBlock *BB, const BasicBlock *join);
	const BranchRegion &getFunctionRegion(Function *F);
	void summarizeInst(Instruction &I, BranchRegion &R,
			   const SmallPtrSetImpl<BasicBlock *> *region);

	/* The branches joined in thread TID, as the current instruction sees them */
	void applyCtrlShadows(Thread &thr, DepInfo &ctrl);

	void updateDataDeps(unsigned int tid, Value *dst, Value *src)
	{
		if (getDepTracker())
			getDepTracker()->updateDataDeps(tid, dst, src);
	}
	void updateDataDeps(unsigned int tid, Value *dst, const DepInfo *e)
	{
		if (getDepTracker())
			getDepTracker()->updateDataDeps(tid, dst, *e);
	}
	void updateDataDeps(unsigned int tid, Value *dst, Event e)
	{
		if (getDepTracker())
			getDepTracker()->updateDataDeps(tid, dst, e);
	}
	void updateAddrPoDeps(unsigned int tid, Value *src)
	{
		if (getDepTracker())
			getDepTracker()->updateAddrPoDeps(tid, src);
	}
	void updateCtrlDeps(unsigned int tid, Value *src)
	{
		if (getDepTracker())
			getDepTracker()->updateCtrlDeps(tid, src);
	}
	void updateCtrlDeps(unsigned int tid, Event e)
	{
		if (getDepTracker())
			getDepTracker()->updateCtrlDeps(tid, e);
	}

	std::unique_ptr<EventDeps> updateFunArgDeps(unsigned int tid, Function *F);
	void updateInternalFunRetDeps(unsigned int tid, Function *F, Instruction *CS);

	void setThreadArgDeps(unsigned int id);

	void clearDeps(unsigned int tid)
	{
		if (getDepTracker())
			getDepTracker()->clearDeps(tid);
	}

	/* Gets naming information for value V (or value with key KEY), if it is
	 * an internal variable with no value correspondence */
	const NameInfo *getVarNameInfo(Value *v, StorageDuration sd, AddressSpace spc,
				       const VariableInfo<ModuleID::ID>::InternalKey &key = {});
}; // namespace llvm

} // namespace llvm

/** Make `llvm::Thread` formattable with `std::format`. */
template <> struct std::formatter<llvm::Thread> {
	constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

	auto format(const llvm::Thread &thr, std::format_context &ctx) const
	{
		return std::format_to(ctx.out(), "<{}, {}> {}", thr.parentId, thr.id,
				      thr.threadFun->getName().str());
	}
};

#endif
