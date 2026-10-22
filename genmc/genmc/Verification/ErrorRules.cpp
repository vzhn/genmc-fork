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
#include "genmc/Execution/ModelInterfaces.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Execution/ExecutionState.hpp"
#include "genmc/Execution/GraphUtils.hpp"
#include "genmc/Verification/RfReductions.hpp"

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

/* If LAB accesses another thread's copy of a thread-local (see SAddr),
 * returns that thread, and 0 otherwise. The copies of a thread end with it,
 * as if freed by its end; those of the main thread (0) last as long as the
 * program */
auto threadLocalOwner(const EventLabel *lab) -> int
{
	const auto *mLab = genmc::dyn_cast<MemAccessLabel>(lab);
	if (!mLab || !mLab->getAddr().isStatic())
		return 0;
	auto owner = mLab->getAddr().getThread();
	return owner == lab->getThread() || owner >= lab->getParent()->getNumThreads() ? 0 : owner;
}

auto threadLocalAfterEnd(const GraphModel &model, const EventLabel *lab,
			 const ThreadFinishLabel *eLab) -> std::optional<ErrorDetails>
{
	if (model.getHbView(eLab).contains(lab->getPos()))
		return {};
	return ErrorDetails{
		lab->getPos(), VerificationError::VE_AccessFreed,
		std::format("The access is to a thread-local of thread {}, and does not "
			    "happen before the thread ends\n",
			    eLab->getThread()),
		eLab};
}

auto endedThreadLocal(const GraphModel &model, const ExecutionGraph &g,
		      const ThreadFinishLabel *eLab) -> std::optional<ErrorDetails>
{
	/* (The main thread's copies last as long as the program; an owner of 0
	 * is no owner, see threadLocalOwner()) */
	if (eLab->getThread() == 0 || !g.getState().hasThreadLocals())
		return {};

	for (const auto &lab : g.labels()) {
		if (threadLocalOwner(&lab) != eLab->getThread())
			continue;
		if (auto err = threadLocalAfterEnd(model, &lab, eLab); err)
			return err;
	}
	return {};
}

auto finalWriteViolation(const GraphModel &model, const ExecutionGraph &g, const WriteLabel *wLab)
	-> std::optional<ErrorDetails>
{
	if (!g.hasLocMoreThanOneStore(wLab->getAddr()))
		return {};
	if ((wLab->isFinal() && std::ranges::any_of(g.co(wLab->getAddr()),
						    [&](auto &sLab) {
							    return !model.getHbView(wLab).contains(
								    sLab.getPos());
						    })) ||
	    (!wLab->isFinal() && std::ranges::any_of(g.co(wLab->getAddr()),
						     [&](auto &sLab) { return sLab.isFinal(); }))) {
		return ErrorDetails{wLab->getPos(), VerificationError::VE_Annotation,
				    "Multiple stores at final location!"};
	}
	return {};
}

auto iprUnorderedWrites(const ExecutionGraph &g, const ReadLabel *rLab)
	-> std::optional<ErrorDetails>
{
	using namespace std::string_literals;
	if (!rLab->getAnnot())
		return {};

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
	return ErrorDetails{racyIt->getPos(), VerificationError::VE_WWRace, msg, nullptr, true};
}

auto blindRMWOrderObserved(const GraphModel &model, const ExecutionGraph &g, const ReadLabel *rLab)
	-> std::optional<ErrorDetails>
{
	const BlindRMWFilter blind(g, model);
	if (blind.asCommuting(rLab))
		return {};

	/*
	 * A read that is hb-after one blind FAI but not after an identical one
	 * observes their order: it may see the first without the second.  The
	 * reduction may have fixed that order, so such a read is not supported.
	 */
	const auto &view = model.getHbView(rLab);
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
	if (valid)
		return {};
	return ErrorDetails{rLab->getPos(), VerificationError::VE_BlindRMWWellFormedness,
			    "This read observes the order of blind RMWs that --blind-rmw-reduction "
			    "treats as unordered. Rerun without --blind-rmw-reduction.\n"};
}

auto blindRMWReorderedWith(const GraphModel &model, const ExecutionGraph &g, const WriteLabel *sLab)
	-> std::optional<ErrorDetails>
{
	/*
	 * The reduction assumed that identical blind FAIs are the only writes to
	 * their location.  If another write is not hb-after one of them, an order
	 * we skipped might have let it revisit that FAI.  (Its prefix is no guide:
	 * the order we kept may have put the FAIs there through their rf chain,
	 * which creates no hb, as they are relaxed.)
	 */
	const BlindRMWFilter blind(g, model);
	const auto *ownRead = getRMWRead(*sLab);
	const auto &view = model.getHbView(sLab);
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
	if (valid)
		return {};
	return ErrorDetails{
		sLab->getPos(), VerificationError::VE_BlindRMWWellFormedness,
		"Blind RMWs on this location are reordered by --blind-rmw-reduction, but "
		"this write is not one of them. Rerun without --blind-rmw-reduction.\n"};
}

auto barrierNotWellFormed(const GraphModel &model, const ExecutionGraph &g,
			  const BIncFaiWriteLabel *sLab) -> std::optional<ErrorDetails>
{
	/* Find the latest round completion (or init, if none exists) */
	auto lastIt = std::ranges::find_if(g.rco(sLab->getAddr()), [sLab](const auto &lab) {
		return &lab != sLab &&
		       ((genmc::isa<BIncFaiWriteLabel>(&lab) &&
			 isLastInBarrierRound(genmc::dyn_cast<BIncFaiWriteLabel>(&lab))) ||
			lab.getOrdering() == MemOrdering::Relaxed);
	});

	/* Check whether the last barrier completion is hb;po;po-before SLAB */
	auto valid = model.getHbView(g.po_imm_pred(sLab)).contains(lastIt->getPos());
	if (valid)
		return {};
	return ErrorDetails{sLab->getPos(), VerificationError::VE_BarrierWellFormedness,
			    "Execution not barrier-well-formed!\n"};
}

} // namespace error_rules
