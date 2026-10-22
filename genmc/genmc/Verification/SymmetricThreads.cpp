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


#include "genmc/Verification/SymmetricThreads.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Support/Logger.hpp"
#include "genmc/Support/ThreadInfo.hpp"
#include "genmc/Verification/Config.hpp"

#include <algorithm>

static auto areThreadPrivateAccesses(const ExecutionGraph &g, int tid, int from, int to) -> bool
{
	/* Without labels for non-atomic accesses, they cannot be told apart */
	if constexpr (!Config::emitNALabels)
		return false;

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

static auto isSymmetric(const ExecutionGraph &g, int candidate, Event parent, const ThreadInfo &info)
	-> bool
{
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
	    !areThreadPrivateAccesses(g, parent.thread, bounds.first + 1, lastMemAccess)) {
		tipSymmetry();
		return false;
	}
	return true;
}

auto symmetricPredecessor(const ExecutionGraph &g, const ThreadCreateLabel *tcLab,
			  const ThreadInfo &childInfo) -> int
{
	auto isOrphan = [&g](int tid) { return !g.getFirstThreadLabel(tid)->getCreate(); };

	/* Has the user provided any info? (A thread whose create was cut has a
	 * stale begin and cannot be a symmetric predecessor) */
	if (childInfo.symmId != -1)
		return isOrphan(childInfo.symmId) ? -1 : childInfo.symmId;

	for (auto i = childInfo.id - 1; i > 0; i--)
		if (!isOrphan(i) && isSymmetric(g, i, tcLab->getPos(), childInfo))
			return i;
	return -1;
}
