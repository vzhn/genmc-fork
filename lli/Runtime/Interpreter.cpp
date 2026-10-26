// For the parts of the code originating from LLVM-3.5:
//===- Interpreter.cpp - Top-Level LLVM Interpreter Implementation --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LLVMLICENSE for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the top-level functionality for the LLVM interpreter.
// This interpreter is designed to be a very simple, portable, inefficient
// interpreter.
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

#include "Runtime/Interpreter.h"
#include "genmc/Support/Error.hpp"
#include "genmc/Support/Parser.hpp"
#include "passes/LLIConfig.hpp"
#include "passes/LLVMUtils.hpp"
#include <algorithm>
#include <llvm/CodeGen/IntrinsicLowering.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Module.h>

#include <cstring>
#include <optional>

using namespace llvm;

extern "C" void LLVMLinkInInterpreter() {}

/// create - Create a new interpreter object.  This can never fail.
///
std::unique_ptr<Interpreter> Interpreter::create(std::unique_ptr<Module> M,
						 std::unique_ptr<ModuleInfo> MI,
						 GenMCDriver *driver, const LLIConfig *userConf,
						 std::string *ErrStr, const Module *canonicalMod)
{
	// Tell this Module to materialize everything and release the GVMaterializer.
	if (Error Err = M->materializeAll()) {
		std::string Msg;
		handleAllErrors(std::move(Err), [&](ErrorInfoBase &EIB) { Msg = EIB.message(); });
		if (ErrStr)
			*ErrStr = Msg;
		// We got an error, just return 0
		return nullptr;
	}
	return std::make_unique<Interpreter>(std::move(M), std::move(MI), driver, userConf,
					     canonicalMod);
}

void Interpreter::resetThread(unsigned int id)
{
	auto &thr = getThrById(id);
	thr.thaw();
	thr.ECStack = {};
	thr.blocked = BlockageType::NotBlocked;
	thr.rng.seed(Thread::seed);
	thr.currDbgInfo = {};
	thr.keyVisit.reset();
	thr.key = nullptr;
	thr.ctrlShadows.clear();

	clearDeps(id);
	setThreadArgDeps(id);

	/* We don't _have to_ figure out the initial action for each thread here (since this is
	 * done by addNewThread() which is called at the beginning of each execution), but
	 * we can do it just in case this is not the case in the future */
	VERIFY(!thr.initEC.empty() || thr.id == 0);
	auto kind = thr.initEC.empty() ? ActionKind::Load
				       : getInstKind(&*thr.initEC.back().CurInst);
	dynState.globalInstructions[thr.id] = {kind, Event(thr.id, 0)};
}

void Interpreter::setThreadArgDeps(unsigned int id)
{
	/* A thread's argument depends on its start, as the start on the
	 * create, which depends on the argument passed (LKMM's SRCU follows
	 * these, as data dependencies through memory, to match an unlock) */
	auto &thr = getThrById(id);
	if (thr.id == 0 || thr.initEC.empty())
		return;
	auto *F = thr.initEC.back().CurFunction;
	if (F && !F->arg_empty())
		updateDataDeps(id, &*F->arg_begin(), Event(id, 0));
}

/* The dynamic state of the interpreter at an event (see ResumePoint):
 * everything but the naming information, which error replays rebuild. The
 * threads that did not run since an earlier resume point share its copies */
struct InterpreterResumePoint : public ResumePoint {
	std::vector<std::shared_ptr<const Thread>> threads;
	std::vector<Action> globalInstructions;
	int currentThread{};
	value_ptr<DepTracker, DepTrackerCloner> depTracker = nullptr;
	ProgramState programState{};
	GenericValue exitValue;
	std::vector<Function *> atExitHandlers;

	[[nodiscard]] auto threadPositions() const -> std::vector<Event> override
	{
		std::vector<Event> positions;
		positions.reserve(globalInstructions.size());
		for (const auto &action : globalInstructions)
			positions.push_back(action.event);
		return positions;
	}
};

auto Interpreter::saveResumePoint(Event pos) -> std::shared_ptr<ResumePoint>
{
	/* The current thread must be adding the event from within an
	 * instruction. The state saved is the one before the instruction, which
	 * runs again on resume: it adds its events (an RMW adds two, with a
	 * driver call each) at their positions again, driven by the graph */
	if (!callingDriver_ || pos.thread != dynState.currentThread ||
	    pos.index <= visitStartPos_.index || pos.index > visitStartPos_.index + 2 ||
	    ECStack().empty())
		return nullptr;

	auto rp = std::make_shared<InterpreterResumePoint>();
	rp->threads.reserve(dynState.threads.size());
	for (auto &thr : dynState.threads) {
		if (thr.id == dynState.currentThread) {
			/* Mid-instruction: the instruction runs again on resume */
			auto copy = std::make_shared<Thread>(thr);
			copy->frozen = nullptr;
			--copy->ECStack.back().CurInst;
			rp->threads.push_back(std::move(copy));
			continue;
		}
		if (!thr.frozen) {
			auto copy = std::make_shared<Thread>(thr);
			copy->frozen = nullptr;
			thr.frozen = std::move(copy);
		}
		rp->threads.push_back(thr.frozen);
	}
	rp->globalInstructions = dynState.globalInstructions;
	rp->globalInstructions[dynState.currentThread].event = visitStartPos_;
	rp->currentThread = dynState.currentThread;
	rp->depTracker = dynState.depTracker;
	rp->programState = dynState.programState;
	rp->exitValue = dynState.ExitValue;
	rp->atExitHandlers = dynState.AtExitHandlers;
	return rp;
}

auto Interpreter::resumeFrom(const ResumePoint &rp, const std::vector<ThreadInfo> &tis,
			     const std::vector<int> &lastIdx) -> bool
{
	const auto &irp = static_cast<const InterpreterResumePoint &>(rp);
	if (irp.threads.size() > lastIdx.size())
		return false;
	for (std::size_t i = 0; i < irp.threads.size(); i++)
		if (irp.globalInstructions[i].event.index > lastIdx[i])
			return false;

	dynState.threads.resize(irp.threads.size());
	for (std::size_t i = 0; i < irp.threads.size(); i++) {
		dynState.threads[i] = *irp.threads[i];
		/* Until it runs, the thread's state is still the saved one */
		dynState.threads[i].frozen = irp.threads[i];
	}
	dynState.globalInstructions = irp.globalInstructions;
	dynState.currentThread = irp.currentThread;
	dynState.depTracker = irp.depTracker;
	dynState.programState = irp.programState;
	dynState.ExitValue = irp.exitValue;
	dynState.AtExitHandlers = irp.atExitHandlers;
	dynState.collectDbgInfo = false;
	dynState.nameInfo.clear();
	dynState.heapSites.clear();
	for (const auto &ti : tis)
		if (ti.id >= getNumThreads())
			constructAddThreadFromInfo(ti);
	run();
	return true;
}

void Interpreter::reset()
{
	/*
	 * Make sure that all execution stacks are empty since there may
	 * have been a failed assume on some thread and a join waiting on
	 * that thread (joins do not empty ECStacks)
	 */
	dynState.currentThread = 0;
	dynState.collectDbgInfo = false;
	dynState.nameInfo.clear();
	dynState.heapSites.clear();
	dynState.AtExitHandlers.clear();
	std::for_each(threads_begin(), threads_end(), [this](Thread &thr) { resetThread(thr.id); });
}

Thread &Interpreter::addNewThread(Thread &&thread)
{
	/* In case of a replay, do nothing (so that we don't have to replay in porf) */
	if (thread.id < getNumThreads()) {
		auto &exstThr = dynState.threads[thread.id];
		VERIFY(exstThr.id == thread.id);
		if (exstThr.parentId == thread.parentId && exstThr.threadFun == thread.threadFun &&
		    exstThr.threadArg == thread.threadArg)
			return exstThr;
		/* The driver revived a thread whose begin outlived its create: the
		 * entry was set up from the stale begin and has not run (the begin
		 * was terminated), so replace it. (Any other mismatch is a replay
		 * divergence) */
		VERIFY(exstThr.ECStack.size() == exstThr.initEC.size() &&
		       exstThr.ECStack.back().CurInst == exstThr.initEC.back().CurInst &&
		       dynState.globalInstructions[thread.id].event.index == 0);
		auto id = thread.id;
		exstThr = std::move(thread);
		dynState.globalInstructions[id] = {getInstKind(&*exstThr.initEC.back().CurInst),
						   Event(id, 0)};
		return exstThr;
	}

	VERIFY(thread.id == getNumThreads());
	dynState.threads.push_back(std::move(thread));

	auto &thr = dynState.threads.back();
	VERIFY(!thr.initEC.empty() || thr.id == 0);
	auto kind = thr.initEC.empty() ? ActionKind::Load
				       : getInstKind(&*thr.initEC.back().CurInst);
	dynState.globalInstructions.emplace_back(kind, Event(thr.id, 0));
	return thr;
}

/* Creates an entry for the main() function */
Thread &Interpreter::createAddMainThread()
{
	Thread main(mainFun, 0);
	dynState.threads.clear(); /* make sure its empty */
	dynState.globalInstructions.clear();
	return addNewThread(std::move(main));
}

/* Creates an entry for another thread */
Thread &Interpreter::createAddNewThread(llvm::Function *F, SVal arg, int tid, int pid,
					const llvm::ExecutionContext &SF)
{
	Thread thr(F, arg, tid, pid, SF);
	thr.ECStack.push_back(SF);
	return addNewThread(std::move(thr));
}

void Interpreter::collectStaticAddresses()
{
	auto *M = Modules.back().get();
	std::vector<std::pair<const GlobalVariable *, void *>> toReinitialize;

	for (auto &v : GLOBALS(*M)) {
		char *ptr = static_cast<char *>(GVTOP(getConstantValue(&v)));
		const uint64_t typeSize = getDataLayout().getTypeAllocSize(v.getValueType());

		/* A zero-sized global (an empty struct, a zero-length array, or
		 * the `external global [0 x ptr]` vtable that C++ type info
		 * refers to) still needs an address of its own: reserve one
		 * byte for it, reading as zero, as for malloc(0) */
		static const char zeroByte = 0;
		const uint64_t allocSize = std::max<uint64_t>(typeSize, 1);

		/* Allocate a symbolic address for this global via the driver API. A
		 * thread-local has a copy per thread, each starting as the
		 * initializer: this is the main thread's (see getOperandValue()) */
		auto addr = driver->allocateGlobal(ASize(allocSize), v.getAlignment(),
#if EMIT_NA_LABELS
						   typeSize == 0 ? &zeroByte : ptr,
#endif
						   v.getSection() == "__genmc_persist",
						   v.getAddressSpace() == 42, v.isThreadLocal());
		ERROR_ON(addr == SAddr(), "Allocator is out of memory");
		if (v.isThreadLocal())
			threadLocalAddrs_[&v] = addr;
		staticAllocas.insert(std::make_pair(addr, addr + ASize(allocSize - 1)));
		staticValueMap[addr] = ptr;

		/* ... and use that in the EE instead. Make sure to re-initialize it too;
		 * it might contain the address of another global (the EE does not
		 * initialize a thread-local at all). A declaration has no
		 * initializer: the EE bound it to the host's symbol, whose contents
		 * are its initial value */
		updateGlobalMapping(&v, (void *)addr.get());
		if (v.hasInitializer())
			toReinitialize.push_back(std::make_pair(&v, ptr));

		/* Update naming information */
		staticNames[addr] = &v;
		if (!v.hasPrivateLinkage() &&
		    (!MI->idInfo.VID.count(&v) ||
		     !MI->varInfo.globalInfo.count(MI->idInfo.VID.at(&v)))) {
			WARN_ONCE("name-info",
				  "Inadequate naming info for variable {}\nPlease submit a bug "
				  "report to " PACKAGE_BUGREPORT "\n",
				  v.getName().str());
		}
	}

	/* Now that we've updated all mappings, go ahead and re-initialize */
	for (auto &p : toReinitialize)
		InitializeMemory(p.first->getInitializer(), p.second);
}

void Interpreter::setupErrorPolicy(Module *M, const LLIConfig *userConf)
{
	stopOnSystemErrors = !userConf->disableStopOnSystemError;

	auto *errnoVar = M->getGlobalVariable("errno");
	if (!errnoVar)
		return;

	errnoAddr = GVTOP(getConstantValue(errnoVar));
	errnoTyp = errnoVar->getValueType();
	return;
}

std::unique_ptr<EventDeps> Interpreter::makeEventDeps(const DepInfo *addr, const DepInfo *data,
						      const DepInfo *ctrl, const DepInfo *addrPo,
						      const DepInfo *cas)
{
	if (!getDepTracker())
		return nullptr;

	auto result = std::make_unique<EventDeps>();

	if (addr)
		result->addr = *addr;
	if (data)
		result->data = *data;
	if (ctrl) {
		result->ctrl = *ctrl;
		/* Computed with it (getCtrlDeps()) */
		if (scopesCtrlDeps()) {
			result->pctrl = getCurThr().pctrlScratch;
			result->sctrl = getCurThr().sctrlScratch;
		}
	}
	if (addrPo)
		result->addrPo = *addrPo;
	if (cas)
		result->cas = *cas;
	return result;
}

std::unique_ptr<EventDeps> Interpreter::updateFunArgDeps(unsigned int tid, Function *fun)
{
	if (!getDepTracker())
		return nullptr;

	ExecutionContext &SF = ECStack().back();

	/* Handling non-internals is straightforward: the parameters
	 * of the function called get the data dependencies of the
	 * actual arguments */
	auto cacheIt = internalFunCache_.find(fun);
	if (cacheIt == internalFunCache_.end()) {
		auto ai = fun->arg_begin();
		for (auto ci = SF.Caller.arg_begin(), ce = SF.Caller.arg_end(); ci != ce;
		     ++ci, ++ai) {
			updateDataDeps(tid, &*ai, &*ci->get());
		}
		return nullptr;
	}
	auto iFunCode = cacheIt->second;

	/* We have ctrl dependency on the argument of an assume() */
	if (iFunCode == InternalFunctions::Assume) {
		for (auto i = SF.Caller.arg_begin(), e = SF.Caller.arg_end(); i != e; ++i) {
			updateCtrlDeps(tid, *i);
		}
		return nullptr;
	}
	/* We have addr dependency on the argument of mutex/barrier/condvar calls */
	if (isMutexCode(iFunCode) || isCondVarCode(iFunCode) ||
	    iFunCode == InternalFunctions::SrcuReadLock) {
		return makeEventDeps(getDataDeps(tid, *SF.Caller.arg_begin()), nullptr,
				     getCtrlDeps(tid), getAddrPoDeps(tid), nullptr);
	}
	/* srcu_read_unlock() writes its second argument: a data dependency */
	if (iFunCode == InternalFunctions::SrcuReadUnlock) {
		return makeEventDeps(getDataDeps(tid, SF.Caller.arg_begin()[0]),
				     getDataDeps(tid, SF.Caller.arg_begin()[1]), getCtrlDeps(tid),
				     getAddrPoDeps(tid), nullptr);
	}
	return nullptr;
}

bool Interpreter::mayReadThreadId(const llvm::Function *F)
{
	if (auto it = readsThreadId_.find(F); it != readsThreadId_.end())
		return it->second;

	/* (A recursive call does not add anything: assume no while visiting F) */
	readsThreadId_[F] = false;
	auto reads = false;
	for (const auto &I : llvm::instructions(*F)) {
		const auto *CB = llvm::dyn_cast<llvm::CallBase>(&I);
		if (!CB || CB->isInlineAsm())
			continue;
		const auto *callee = CB->getCalledFunction();
		if (callee == nullptr) {
			reads = true; /* an indirect call */
			break;
		}
		auto it = internalFunCache_.find(callee);
		if (it != internalFunCache_.end()) {
			if (it->second == InternalFunctions::ThreadSelf) {
				reads = true;
				break;
			}
			continue;
		}
		if (!callee->isDeclaration() && mayReadThreadId(callee)) {
			reads = true;
			break;
		}
	}
	readsThreadId_[F] = reads;
	return reads;
}

void Interpreter::updateInternalFunRetDeps(unsigned int tid, Function *F, Instruction *CS)
{
	auto it = internalFunCache_.find(F);
	if (it == internalFunCache_.end())
		return;

	/* The value a join returns depends on the join, as the join on the
	 * thread's end, which depends on the value returned */
	if (isAllocCode(it->second) || it->second == InternalFunctions::SrcuReadLock ||
	    it->second == InternalFunctions::ThreadJoin)
		updateDataDeps(tid, CS, threadPos(tid));
}

static std::string getFilenameFromMData(const llvm::DILocation &loc)
{
	llvm::StringRef file = loc.getFilename();
	llvm::StringRef dir = loc.getDirectory();

	VERIFY(file.size());

	std::string absPath;
	if (file.front() == '/') {
		absPath = file.str();
	} else {
		VERIFY(dir.size());
		absPath = dir.str();
		if (absPath.back() != '/')
			absPath += "/";
		absPath += file;
	}
	return absPath;
}

std::string Interpreter::getAccessedVarName(const std::optional<SAddr> &loc)
{
	if (!loc)
		return "";
	if (loc->isStatic())
		return getStaticName(*loc);

	auto it = dynState.nameInfo.find(*loc);
	if (it == dynState.nameInfo.end())
		return "";
	const auto &[name, fields, elemSize] = it.value();
	auto offset = *loc - it.start();
	std::string index;
	if (elemSize != 0 && it.stop() - it.start() > elemSize) {
		index = std::format("[{}]", offset / elemSize);
		offset %= elemSize;
	}
	if (fields && !fields->empty())
		return name + index + fields->getNameAtOffset(offset);
	return name + index + (offset != 0 ? std::format("+{}", offset) : "");
}

auto Interpreter::isRuntimeHeader(const llvm::DILocation &loc) -> bool
{
	auto [it, inserted] = runtimeHeaders_.try_emplace(loc.getFile(), false);
	if (!inserted)
		return it->second;

	auto dir =
		std::filesystem::path(getFilenameFromMData(loc)).parent_path().lexically_normal();
	/* (The program may name a directory differently, e.g., through a symlink) */
	it->second = std::ranges::any_of(runtimeIncludeDirs_, [&](const auto &inc) {
		std::error_code ec;
		return dir == inc || std::filesystem::equivalent(dir, inc, ec);
	});
	return it->second;
}

/* The wrappers of the runtime headers (pthread_mutex_lock(), free(), ...)
 * are inlined where the program calls them, or called from there: the
 * program's line is the first one outside the headers, going out through
 * the locations the instruction was inlined at, and then the calls on the
 * stack. Without one, the instruction's own location is reported */
auto Interpreter::getReportedLoc(Thread &thr, const llvm::DISubprogram **callee /* = nullptr */)
	-> const llvm::DILocation *
{
	const llvm::DILocation *own = nullptr;
	const llvm::DILocation *header = nullptr;
	for (auto i = thr.ECStack.size(); i-- > 0;) {
		auto &SF = thr.ECStack[i];
		auto *I = i + 1 == thr.ECStack.size() ? SF.CurInst->getPrevNode() : &SF.Caller;
		const auto *loc = I ? I->getDebugLoc().get() : nullptr;
		if (!loc)
			break;
		for (; loc; loc = loc->getInlinedAt()) {
			if (!isRuntimeHeader(*loc)) {
				/* (The program called the wrapper the last header line is in) */
				if (callee)
					*callee = header ? header->getScope()->getSubprogram()
							 : nullptr;
				return loc;
			}
			header = loc;
			if (!own)
				own = loc;
		}
	}
	return own;
}

/* A heap block is named after the program's call that allocated it (e.g.,
 * calloc@locs.c:18), numbered if the call made several, with the fields of
 * the type its result is stored as, if known (see MDataInfo::collectHeapInfo()) */
void Interpreter::nameHeapBlock(Function *F, SVal addr, uint64_t size)
{
	if (!dynState.collectDbgInfo)
		return;

	const llvm::DISubprogram *callee = nullptr;
	const auto *loc = getReportedLoc(getCurThr(), &callee);
	if (!loc)
		return;

	auto fun = callee ? callee->getName().str() : F->getName().str();
	if (fun == "_Znwm" || fun == "_Znam")
		fun = "new";
	auto name = std::format("{}@{}:{}", fun,
				std::filesystem::path(loc->getFilename().str()).filename().string(),
				loc->getLine());
	if (auto n = ++dynState.heapSites[name]; n > 1)
		name += std::format("#{}", n);

	auto block = BlockName{.name = name};
	const auto *call = getCurThr().ECStack.back().CurInst->getPrevNode();
	if (auto key = call ? getHeapInfoKey(*call) : std::nullopt; key) {
		const auto &info = MI->varInfo.heapInfo.at(*key);
		block.fields = info.fields.get();
		block.elemSize = info.size;
	}
	dynState.nameInfo.insert(SAddr(addr.get()), SAddr(addr.get()) + std::max<uint64_t>(size, 1),
				 block);
}

const GenMCDriver::EventDbgInfo *Interpreter::currDbgInfo(std::optional<SAddr> loc)
{
	auto &thr = getCurThr();
	if (!dynState.collectDbgInfo || thr.ECStack.empty())
		return nullptr;

	const auto *dl = getReportedLoc(thr);
	if (!dl)
		return nullptr;

	thr.currDbgInfo.line = dl->getLine();
	thr.currDbgInfo.file = getFilenameFromMData(*dl);
	thr.currDbgInfo.functionName = thr.threadFun->getName().str();
	thr.currDbgInfo.source =
		genmc::getFileLineByNumber(thr.currDbgInfo.file, thr.currDbgInfo.line);
	thr.currDbgInfo.accessedVarName = getAccessedVarName(loc);

	/* Prettify strings (after all data has been parsed) */
	genmc::stripWhitespace(thr.currDbgInfo.source);
	genmc::extractFilename(thr.currDbgInfo.file);
	return &thr.currDbgInfo;
}

//===----------------------------------------------------------------------===//
// Interpreter ctor - Initialize stuff
//
Interpreter::Interpreter(std::unique_ptr<Module> M, std::unique_ptr<ModuleInfo> MI,
			 GenMCDriver *driver, const LLIConfig *userConf, const Module *canonicalMod)
	: ExecutionEngine(std::move(M)), MI(std::move(MI)), driver(driver)
{
	/* Before anything takes a function's address (emitGlobals() below) */
	if (canonicalMod && canonicalMod != Modules.back().get()) {
		auto &local = Modules.back()->getFunctionList();
		const auto &canon = canonicalMod->getFunctionList();
		VERIFY(local.size() == canon.size());
		auto cit = canon.begin();
		for (auto &F : local) {
			/* A bitcode round trip keeps the function list and its order */
			VERIFY(F.getName() == cit->getName());
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
			auto *addr = static_cast<void *>(const_cast<Function *>(&*cit));
			funToCanon_[&F] = addr;
			canonToFun_[addr] = &F;
			++cit;
		}
	}

	for (const auto *dirs : {&userConf->runtimeIncludeDirs, &userConf->libraryDirs}) {
		for (const auto &dir : *dirs) {
			auto path = std::filesystem::path(dir).lexically_normal();
			runtimeIncludeDirs_.push_back(path.has_filename() ? path
									  : path.parent_path());
		}
	}

	memset(&dynState.ExitValue.Untyped, 0, sizeof(dynState.ExitValue.Untyped));

	// Initialize the "backend"
	initializeExecutionEngine();
	initializeExternalFunctions();
	emitGlobals();

	IL = new IntrinsicLowering(getDataLayout());

	auto mod = Modules.back().get();

	/* Cache internal func codes by ptr, so the hot-call path doesn't build strings */
	for (auto &F : *mod) {
		if (auto it = internalFunNames.find(F.getName().str());
		    it != internalFunNames.end())
			internalFunCache_.try_emplace(&F, it->second);
	}

	/* Number the instructions for event keys (the order survives the
	 * copies of the module that the workers of a thread pool get) */
	if (driver->checksReplayKeys()) {
		std::uint32_t id = 0;
		for (auto &F : *mod)
			for (auto &I : instructions(F))
				keyInstIds_[&I] = ++id;
	}

	/* Set up a dependency tracker if the model requires it */
	if (userConf->isDepTrackingModel)
		dynState.depTracker = std::make_unique<DepTracker>();

	collectStaticAddresses();

	/* Set up the system error policy */
	setupErrorPolicy(mod, userConf);

	/* Setup the interpreter for the exploration */
	mainFun = FindFunctionNamed(userConf->programEntryFun);
	ERROR_ON(!mainFun, "Could not find program's entry point function!");

	createAddMainThread();
}

Interpreter::~Interpreter() { delete IL; }

namespace {
class ArgvArray {
	std::unique_ptr<char[]> Array;
	std::vector<std::unique_ptr<char[]>> Values;

public:
	/// Turn a vector of strings into a nice argv style array of pointers to null
	/// terminated strings.
	void *reset(LLVMContext &C, ExecutionEngine *EE, const std::vector<std::string> &InputArgv);
};
} // anonymous namespace
void *ArgvArray::reset(LLVMContext &C, ExecutionEngine *EE,
		       const std::vector<std::string> &InputArgv)
{
	Values.clear(); // Free the old contents.
	Values.reserve(InputArgv.size());
	unsigned PtrSize = EE->getDataLayout().getPointerSize();
	Array = std::make_unique<char[]>((InputArgv.size() + 1) * PtrSize);

	// LLVM_DEBUG(dbgs() << "JIT: ARGV = " << (void *)Array.get() << "\n");
	Type *SBytePtr = PointerType::getUnqual(C);

	for (unsigned i = 0; i != InputArgv.size(); ++i) {
		unsigned Size = InputArgv[i].size() + 1;
		auto Dest = std::make_unique<char[]>(Size);
		// LLVM_DEBUG(dbgs() << "JIT: ARGV[" << i << "] = " << (void *)Dest.get()
		//                   << "\n");

		std::copy(InputArgv[i].begin(), InputArgv[i].end(), Dest.get());
		Dest[Size - 1] = 0;

		// Endian safe: Array[i] = (PointerTy)Dest;
		EE->StoreValueToMemory(PTOGV(Dest.get()), (GenericValue *)(&Array[i * PtrSize]),
				       SBytePtr);
		Values.push_back(std::move(Dest));
	}

	// Null terminate it
	EE->StoreValueToMemory(PTOGV(nullptr), (GenericValue *)(&Array[InputArgv.size() * PtrSize]),
			       SBytePtr);
	return Array.get();
}

void Interpreter::setupFunctionCall(Function *F, ArrayRef<GenericValue> ArgValues)
{
	// Try extra hard not to pass extra args to a function that isn't
	// expecting them.  C programmers frequently bend the rules and
	// declare main() with fewer parameters than it actually gets
	// passed, and the interpreter barfs if you pass a function more
	// parameters than it is declared to take. This does not attempt to
	// take into account gratuitous differences in declared types,
	// though.
	std::vector<GenericValue> ActualArgs;
	const unsigned ArgCount = F->getFunctionType()->getNumParams();
	for (unsigned i = 0; i < ArgCount; ++i)
		ActualArgs.push_back(ArgValues[i]);

	callFunction(F, ActualArgs, nullptr);
}

/// run - Start execution with the specified function and arguments.
///
GenericValue Interpreter::runFunction(Function *F, ArrayRef<GenericValue> ArgValues)
{
	assert(F && "Function *F was null at entry to run()");

	// Set up the function call.
	setupFunctionCall(F, ArgValues);

	// Start executing the function.
	run();

	return dynState.ExitValue;
}

void Interpreter::setupStaticCtorsDtors(Module &module, bool isDtors)
{
	StringRef Name(isDtors ? "llvm.global_dtors" : "llvm.global_ctors");
	GlobalVariable *GV = module.getNamedGlobal(Name);

	// If this global has internal linkage, or if it has a use, then it must be
	// an old-style (llvmgcc3) static ctor with __main linked in and in use.  If
	// this is the case, don't execute any of the global ctors, __main will do
	// it.
	if (!GV || GV->isDeclaration() || GV->hasLocalLinkage())
		return;

	// Should be an array of '{ i32, void ()* }' structs.  The first value is
	// the init priority, which we ignore.
	ConstantArray *InitList = dyn_cast<ConstantArray>(GV->getInitializer());
	if (!InitList)
		return;
	for (unsigned i = 0, e = InitList->getNumOperands(); i != e; ++i) {
		ConstantStruct *CS = dyn_cast<ConstantStruct>(InitList->getOperand(i));
		if (!CS)
			continue;

		Constant *FP = CS->getOperand(1);
		if (FP->isNullValue())
			continue; // Found a sentinal value, ignore.

		// Strip off constant expression casts.
		if (ConstantExpr *CE = dyn_cast<ConstantExpr>(FP))
			if (CE->isCast())
				FP = CE->getOperand(0);

		// Setup the ctor/dtor SF and quit
		if (Function *F = dyn_cast<Function>(FP))
			setupFunctionCall(F, {});

		// FIXME: It is marginally lame that we just do nothing here if we see an
		// entry we don't recognize. It might not be unreasonable for the verifier
		// to not even allow this and just assert here.
	}
}

void Interpreter::setupStaticCtorsDtors(bool isDtors)
{
	// Execute global ctors/dtors for each module in the program.
	for (std::unique_ptr<Module> &M : Modules)
		setupStaticCtorsDtors(*M, isDtors);
}

#ifndef NDEBUG
/// isTargetNullPtr - Return whether the target pointer stored at Loc is null.
static bool isTargetNullPtr(ExecutionEngine *EE, void *Loc)
{
	unsigned PtrSize = EE->getDataLayout().getPointerSize();
	for (unsigned i = 0; i < PtrSize; ++i)
		if (*(i + (uint8_t *)Loc))
			return false;
	return true;
}
#endif

void Interpreter::setupMain(Function *Fn, const std::vector<std::string> &argv,
			    const char *const *envp)
{
	std::vector<GenericValue> GVArgs;
	GenericValue GVArgc;
	GVArgc.IntVal = APInt(32, argv.size());

	// Check main() type
	unsigned NumArgs = Fn->getFunctionType()->getNumParams();
	FunctionType *FTy = Fn->getFunctionType();
	Type *PPInt8Ty = PointerType::get(Fn->getContext(), 0);

	// Check the argument types.
	if (NumArgs > 3)
		report_fatal_error("Invalid number of arguments of main() supplied");
	if (NumArgs >= 3 && FTy->getParamType(2) != PPInt8Ty)
		report_fatal_error("Invalid type for third argument of main() supplied");
	if (NumArgs >= 2 && FTy->getParamType(1) != PPInt8Ty)
		report_fatal_error("Invalid type for second argument of main() supplied");
	if (NumArgs >= 1 && !FTy->getParamType(0)->isIntegerTy(32))
		report_fatal_error("Invalid type for first argument of main() supplied");
	if (!FTy->getReturnType()->isIntegerTy() && !FTy->getReturnType()->isVoidTy())
		report_fatal_error("Invalid return type of main() supplied");

	ArgvArray CArgv;
	ArgvArray CEnv;
	if (NumArgs) {
		GVArgs.push_back(GVArgc); // Arg #0 = argc.
		if (NumArgs > 1) {
			// Arg #1 = argv.
			GVArgs.push_back(PTOGV(CArgv.reset(Fn->getContext(), this, argv)));
			assert(!isTargetNullPtr(this, GVTOP(GVArgs[1])) &&
			       "argv[0] was null after CreateArgv");
			if (NumArgs > 2) {
				std::vector<std::string> EnvVars;
				for (unsigned i = 0; envp[i]; ++i)
					EnvVars.emplace_back(envp[i]);
				// Arg #2 = envp.
				GVArgs.push_back(
					PTOGV(CEnv.reset(Fn->getContext(), this, EnvVars)));
			}
		}
	}

	setupFunctionCall(Fn, GVArgs);
}
