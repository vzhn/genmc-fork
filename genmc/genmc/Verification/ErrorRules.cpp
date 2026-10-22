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


#include "genmc/Verification/ErrorRules.hpp"
#include "genmc/Execution/Consistency/ModelInterfaces.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Execution/ExecutionState.hpp"
#include "genmc/Execution/GraphUtils.hpp"

#include <algorithm>
#include <climits>
#include <ranges>
#include <utility>

namespace error_rules {

auto invalidFreeReason(const ExecutionState &state, SAddr loc, StorageDuration sdur)
	-> std::optional<std::string>
{
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

auto invalidAccess(const ExecutionState &state, bool staticChecks, Event pos,
		   const AAccess &access) -> std::optional<ErrorDetails>
{
	/* Check that static accesses hit a registered global range,
	 * and dynamic accesses hit allocated memory */
	if ((staticChecks && !access.addr.isDynamic() && !state.isStatic(access.addr)) ||
	    (access.addr.isDynamic() && !state.isAccessible(access.addr)))
		return ErrorDetails{pos, VerificationError::VE_AccessNonMalloc};
	return std::nullopt;
}

auto uninitializedRead(const GraphModel &model, const ReadLabel *rLab)
	-> std::optional<ErrorDetails>
{
	// FIXME: Have label for mutex-destroy and check type instead of val.
	//        Also for barriers.

	/* Dynamic accesses should read initialized memory (if reading from INIT).
	 * We use two separate checks for atomics and NAs, since mixed-size NAs don't have an rf */
	if (rLab->getAddr().isDynamic() &&
	    (!rLab->getParent()->getState().isInitialized(rLab->getAccess()) ||
	     !model.getHbView(rLab).contains(
		     rLab->getParent()->getState().getInitView(rLab->getAccess()))))
		return ErrorDetails{rLab->getPos(), VerificationError::VE_UninitializedMem};

	/* Slightly unrelated check, but ensure there are no mixed-size accesses */
	if (rLab->getRf() && !rLab->getRf()->getPos().isInitializer() &&
	    genmc::dyn_cast<WriteLabel>(rLab->getRf())->getSize() != rLab->getSize())
		return ErrorDetails{rLab->getPos(), VerificationError::VE_MixedSize,
				    "Mixed-size accesses detected: tried to read with a " +
					    std::to_string(rLab->getSize().get() * CHAR_BIT) +
					    "-bit access!\n" + "Please check the LLVM-IR.\n"};
	return std::nullopt;
}

auto invalidUnlock(const WriteLabel *wLab) -> std::optional<ErrorDetails>
{
	/* Unlocks should unlock mutexes locked by the same thread */
	const auto *uLab = genmc::dyn_cast<UnlockWriteLabel>(wLab);
	if (uLab && !findMatchingLock(uLab))
		return ErrorDetails{uLab->getPos(), VerificationError::VE_InvalidUnlock,
				    "Called unlock() on mutex not locked by the same thread!"};
	return std::nullopt;
}

auto mixedSizeAccess(const MemAccessLabel *lab) -> std::optional<ErrorDetails>
{
	if (lab->isNotAtomic())
		return std::nullopt;
	if (lab->getParent()->getState().isAtomicAccessConsistent(lab->getAccess()))
		return std::nullopt;
	return ErrorDetails{lab->getPos(), VerificationError::VE_MixedSize,
			    "Mixed-size accesses detected: tried to read with a " +
				    std::to_string(lab->getSize().get() * CHAR_BIT) +
				    "-bit access!\n"};
}

auto wwRaceUpgrade(const ExecutionGraph &g, Event pos, VerificationError wcode,
		   bool symmetryReduction, bool ipr) -> WarningUpgrade
{
	using namespace std::string_literals;
	if (wcode != VerificationError::VE_WWRace)
		return {};
	if (!symmetryReduction && !ipr)
		return {};

	const auto *lab = g.getEventLabel(pos);
	auto isSymmetric = [&](auto tid) {
		return g.getFirstThreadLabel(tid)->getSymmPredTid() != -1;
	};
	auto srUpgrade = symmetryReduction && std::ranges::any_of(g.thr_ids(), isSymmetric);
	auto iprUpgrade = ipr && std::ranges::any_of(g.samelocs(lab), [&](auto &oLab) {
				  const auto *rLab = genmc::dyn_cast<ReadLabel>(&oLab);
				  return rLab && rLab->getAnnot();
			  });
	WarningUpgrade res;
	res.upgrade = srUpgrade || iprUpgrade;

	/* Without the symmetries GenMC inferred itself (rather than the user
	 * declared), this would only be a warning: verification may be
	 * repeated without SR. IPR is GenMC's own optimization: verification
	 * may also be repeated without it */
	if (res.upgrade) {
		res.iprError = iprUpgrade;
		res.inferredSymmetryError =
			!iprUpgrade && std::ranges::none_of(g.thr_ids(), [&](auto tid) {
				return isSymmetric(tid) &&
				       g.getFirstThreadLabel(tid)->getThreadInfo().symmId != -1;
			});
	}
	auto [cause, cli] = iprUpgrade
				    ? std::make_pair("in-place revisiting (IPR)"s, "-disable-ipr"s)
				    : std::make_pair("symmetry reduction (SR)"s, "-disable-sr"s);
	res.msg = "Unordered writes do not constitute a bug per se, though they often "
		  "indicate faulty design.\n" +
		  (res.upgrade ? ("This warning is treated as an error due to " + cause +
				  ".\n"
				  "You can use " +
				  cli + " to disable these features."s)
			       : ""s);
	return res;
}

} // namespace error_rules
