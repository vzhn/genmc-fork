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

/*******************************************************************************
 * CAUTION: This file is generated automatically by Kater -- DO NOT EDIT.
 * From kat/lkmm-genmc.kat, Kater 83d2d1c-dirty (scripts/lkmm/kater-export.sh).
 *******************************************************************************/

// NOLINTBEGIN
#include "LKMMChecker.hpp"
#include "genmc/ADT/DepView.hpp"
#include "genmc/ADT/VSet.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Execution/GraphUtils.hpp"
#include "genmc/Verification/Config.hpp"
#include "genmc/Verification/VerificationError.hpp"

#include <algorithm>

bool LKMMChecker::isDepTracking() const { return 1; }

bool LKMMChecker::visitCalc113Iterative(std::vector<DFSWorklistEntry> &worklist,
					View &calcRes) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		case 0: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();

			break;
		}
		case 1: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = lab; true)
				if (calcRes.update(pLab->view(0)); true) {
					worklist.emplace_back(0, pLab);
				}

			break;
		}
		case 2: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true)
					if (calcRes.updateIdx(pLab->getPos()); true) {
						worklist.emplace_back(0, pLab);
					}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (calcRes.updateIdx(pLab->getPos()); true) {
					worklist.emplace_back(0, pLab);
				}
			if (auto pLab = g.tc_pred(lab); pLab)
				if (calcRes.updateIdx(pLab->getPos()); true) {
					worklist.emplace_back(0, pLab);
				}
			if (auto pLab = g.tj_pred(lab); pLab)
				if (calcRes.updateIdx(pLab->getPos()); true) {
					worklist.emplace_back(0, pLab);
				}
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					worklist.emplace_back(1, pLab);
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				worklist.emplace_back(1, pLab);
			}
			if (auto pLab = g.tc_pred(lab); pLab) {
				worklist.emplace_back(1, pLab);
			}
			if (auto pLab = g.tj_pred(lab); pLab) {
				worklist.emplace_back(1, pLab);
			}

			break;
		}
		default:
			UNREACHABLE();
		}
	}
	return true;
}

View LKMMChecker::visitCalc113(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();
	View calcRes;

	calcRes.updateIdx(lab->getPos());

	/* Explore from all accepting states using DFS */
	std::vector<DFSWorklistEntry> startStates = {
		{2, lab},
	};

	visitCalc113Iterative(startStates, calcRes);
	return calcRes;
}

auto LKMMChecker::checkCalc113(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	return visitCalc113(lab);
}

bool LKMMChecker::visitCalc114Iterative(std::vector<DFSWorklistEntry> &worklist,
					View &calcRes) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		case 0: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();

			break;
		}
		case 1: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = lab; true)
				if (calcRes.update(pLab->view(1)); true) {
					worklist.emplace_back(0, pLab);
				}

			break;
		}
		case 2: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true)
					if (calcRes.updateIdx(pLab->getPos()); true) {
						worklist.emplace_back(0, pLab);
					}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (calcRes.updateIdx(pLab->getPos()); true) {
					worklist.emplace_back(0, pLab);
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (calcRes.updateIdx(pLab->getPos()); true) {
					if (!genmc::isa<InitLabel>(pLab)) {
						worklist.emplace_back(0, pLab);
					}
				}
			if (auto pLab = g.tc_pred(lab); pLab)
				if (calcRes.updateIdx(pLab->getPos()); true) {
					worklist.emplace_back(0, pLab);
				}
			if (auto pLab = g.tj_pred(lab); pLab)
				if (calcRes.updateIdx(pLab->getPos()); true) {
					worklist.emplace_back(0, pLab);
				}
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					worklist.emplace_back(1, pLab);
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				worklist.emplace_back(1, pLab);
			}
			if (auto *iLab = genmc::dyn_cast_if_present<InitLabel>(g.rf_pred(lab));
			    iLab) {
				calcRes.update(
					iLab->locView(genmc::cast<ReadLabel>(lab)->getAddr(), 1));
			}
			if (auto pLab = g.rf_pred(lab); pLab) {
				if (!genmc::isa<InitLabel>(pLab)) {
					worklist.emplace_back(1, pLab);
				}
			}
			if (auto pLab = g.tc_pred(lab); pLab) {
				worklist.emplace_back(1, pLab);
			}
			if (auto pLab = g.tj_pred(lab); pLab) {
				worklist.emplace_back(1, pLab);
			}

			break;
		}
		default:
			UNREACHABLE();
		}
	}
	return true;
}

View LKMMChecker::visitCalc114(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();
	View calcRes;

	calcRes.updateIdx(lab->getPos());

	/* Explore from all accepting states using DFS */
	std::vector<DFSWorklistEntry> startStates = {
		{2, lab},
	};

	visitCalc114Iterative(startStates, calcRes);
	return calcRes;
}

auto LKMMChecker::checkCalc114(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	return visitCalc114(lab);
}

void LKMMChecker::calculateSaved([[maybe_unused]] EventLabel *lab) {}

void LKMMChecker::calculateViews(EventLabel *lab)
{
	lab->setViews({});

	{
		lab->addView({});
		lab->setView(std::move(checkCalc113(lab)), 0);
	}

	{
		lab->addView({});
		lab->setView(std::move(checkCalc114(lab)), 1);
	}
}

void LKMMChecker::updateMMViews(EventLabel *lab)
{
	calculateViews(lab);
	calculateSaved(lab);
}

static auto isWriteRfBefore(const WriteLabel *wLab, const EventLabel *lab) -> bool
{
	auto &before = lab->view(0);
	return before.contains(wLab->getPos()) ||
	       std::ranges::any_of(wLab->readers(),
				   [&](auto &rLab) { return before.contains(rLab.getPos()); });
}

static auto isHbOptRfBefore(const EventLabel *lab, const WriteLabel *wLab) -> bool
{
	return wLab->view(0).contains(lab->getPos()) ||
	       std::ranges::any_of(wLab->readers(), [&](auto &rLab) {
		       return rLab.view(0).contains(lab->getPos());
	       });
}

static auto splitLocMOBefore(SAddr addr, EventLabel *lab) -> ExecutionGraph::co_iterator
{
	auto &g = *lab->getParent();
	auto rit = std::ranges::find_if(g.rco(addr),
					[&](auto &oLab) { return isWriteRfBefore(&oLab, lab); });
	/* Convert to forward iterator, but be _really_ careful */
	return (rit == std::ranges::end(g.rco(addr))) ? std::ranges::begin(g.co(addr))
						      : ++ExecutionGraph::co_iterator(*rit);
}

static auto splitLocMOAfterHb(ReadLabel *rLab) -> ExecutionGraph::co_iterator
{
	auto &g = *rLab->getParent();
	if (std::ranges::any_of(g.getInitLabel()->rfs(rLab->getAddr()), [rLab](auto &rfLab) {
		    return rfLab.view(0).contains(rLab->getPos());
	    }))
		return std::ranges::begin(g.co(rLab->getAddr()));

	auto it = std::ranges::find_if(g.co(rLab->getAddr()),
				       [&](auto &wLab) { return isHbOptRfBefore(rLab, &wLab); });
	if (it == std::ranges::end(g.co(rLab->getAddr())) || it->view(0).contains(rLab->getPos()))
		return it;
	return ++it;
}

static auto splitLocMOAfter(WriteLabel *wLab) -> ExecutionGraph::co_iterator
{
	auto &g = *wLab->getParent();
	return std::ranges::find_if(g.co(wLab->getAddr()),
				    [&](auto &sLab) { return isHbOptRfBefore(wLab, &sLab); });
}

auto LKMMChecker::getCoherentStores(ReadLabel *rLab) -> std::vector<EventLabel *>
{
	auto &g = *rLab->getParent();
	std::vector<EventLabel *> stores;

	/* Fastpath: co_max(G) is po-before R */
	auto *comaxLab = g.co_max(rLab->getAddr());
	if (comaxLab->getThread() == rLab->getThread() && comaxLab->getIndex() < rLab->getIndex())
		return {comaxLab};

	/*
	 * If there are no stores (rf?;hb)-before the current event
	 * then we can read read from all concurrent stores and the
	 * initializer store. Otherwise, we can read from all concurrent
	 * stores and the mo-latest of the (rf?;hb)-before stores.
	 */
	auto begIt = splitLocMOBefore(rLab->getAddr(), rLab);
	if (begIt == std::ranges::begin(g.co(rLab->getAddr())))
		stores.push_back(g.getInitLabel());
	else {
		stores.push_back(&*(--begIt));
		++begIt;
	}

	/*
	 * If the model supports out-of-order execution we have to also
	 * account for the possibility the read is hb-before some other
	 * store, or some read that reads from a store.
	 */
	auto endIt = (isDepTracking()) ? splitLocMOAfterHb(rLab)
				       : std::ranges::end(g.co(rLab->getAddr()));
	std::transform(begIt, endIt, std::back_inserter(stores), [&](auto &lab) { return &lab; });
	return stores;
}

static auto getMOOptRfAfter(WriteLabel *sLab) -> std::vector<EventLabel *>
{
	auto &g = *sLab->getParent();
	std::vector<EventLabel *> after;
	std::vector<ReadLabel *> rfAfter;

	for (auto &wLab : g.co_succs(sLab)) {
		after.push_back(&wLab);
		std::ranges::transform(wLab.readers(), std::back_inserter(rfAfter),
				       [&](auto &rLab) { return &rLab; });
	}
	std::transform(rfAfter.begin(), rfAfter.end(), std::back_inserter(after),
		       [](auto *rLab) { return rLab; });
	return after;
}

static auto getMOInvOptRfAfter(WriteLabel *sLab) -> std::vector<EventLabel *>
{
	auto &g = *sLab->getParent();
	std::vector<EventLabel *> after;
	std::vector<ReadLabel *> rfAfter;

	/* First, add (mo;rf?)-before */
	for (auto &wLab : g.co_preds(sLab)) {
		after.push_back(&wLab);
		std::ranges::transform(wLab.readers(), std::back_inserter(rfAfter),
				       [&](auto &rLab) { return &rLab; });
	}
	std::transform(rfAfter.begin(), rfAfter.end(), std::back_inserter(after),
		       [](auto *rLab) { return rLab; });

	/* Then, we add the reader list for the initializer */
	for (auto &rLab : g.getInitLabel()->rfs(sLab->getAddr()))
		after.insert(after.end(), &rLab);
	return after;
}

[[maybe_unused]] static auto getRevisitableFrom(WriteLabel *sLab, const VectorClock &pporf,
						WriteLabel *coPred) -> std::vector<ReadLabel *>
{
	const auto *confLab = findPendingRMW(sLab);
	std::vector<ReadLabel *> loads;

	for (auto &rLab : coPred->readers()) {
		if (!pporf.contains(rLab.getPos()) && rLab.getAddr() == sLab->getAddr() &&
		    rLab.isRevisitable() && rLab.wasAddedMax())
			loads.push_back(&rLab);
	}
	if (confLab)
		loads.erase(std::remove_if(loads.begin(), loads.end(),
					   [&](auto &eLab) {
						   return eLab->getStamp() > confLab->getStamp();
					   }),
			    loads.end());
	return loads;
}

void LKMMChecker::filterCoherentRevisits(WriteLabel *sLab, std::vector<ReadLabel *> &ls)
{
	/* If this store is po- and mo-maximal then we are done */
	auto &g = *sLab->getParent();
	if (!isDepTracking() && sLab == g.co_max(sLab->getAddr()))
		return;

	/* First, we have to exclude (mo;rf?;hb?;sb)-after reads */
	auto optRfs = getMOOptRfAfter(sLab);
	ls.erase(std::remove_if(ls.begin(), ls.end(),
				[&](auto &eLab) {
					auto &before = g.po_imm_pred(eLab)->view(0); // hb;sb
					return std::any_of(
						optRfs.begin(), optRfs.end(), [&](auto &evLab) {
							return before.contains(evLab->getPos());
						});
				}),
		 ls.end());

	/* If out-of-order event addition is not supported, then we are done
	 * due to po-maximality */
	if (!isDepTracking())
		return;

	/* Otherwise, we also have to exclude hb-before loads */
	ls.erase(std::remove_if(ls.begin(), ls.end(),
				[&](auto &eLab) { return sLab->view(0).contains(eLab->getPos()); }),
		 ls.end());

	/* ...and also exclude (mo^-1; rf?; (hb^-1)?; sb^-1)-after reads in the *resulting* graph */
	auto &before = sLab->getPrefixView();
	auto moInvOptRfs = getMOInvOptRfAfter(sLab);
	ls.erase(std::remove_if(
			 ls.begin(), ls.end(),
			 [&](auto &eLab) {
				 auto v = g.getViewFromStamp(eLab->getStamp());
				 v->update(before);
				 return std::any_of(
					 moInvOptRfs.begin(), moInvOptRfs.end(), [&](auto &evLab) {
						 return v->contains(evLab->getPos()) && // stays in
											// graph?
							g.po_imm_pred(evLab)->view(0).contains(
								eLab->getPos()); // po-pred to check
										 // evLab != rLab
					 });
			 }),
		 ls.end());
}

auto LKMMChecker::getCoherentPlacings(WriteLabel *wLab) -> std::vector<EventLabel *>
{
	auto &g = *wLab->getParent();
	std::vector<EventLabel *> result;

	/* If it is an RMW store, there is only one possible position in MO */
	if (wLab->isRMW()) {
		auto *rLab = genmc::dyn_cast<ReadLabel>(g.po_imm_pred(wLab));
		VERIFY(rLab);
		auto *rfLab = rLab->getRf();
		VERIFY(rfLab);
		result.push_back(rfLab);
		return result;
	}

	/* Otherwise, we calculate the full range and add the store */
	auto rangeBegin = splitLocMOBefore(wLab->getAddr(), wLab);
	auto rangeEnd = (isDepTracking()) ? splitLocMOAfter(wLab)
					  : std::ranges::end(g.co(wLab->getAddr()));
	auto cos = std::ranges::subrange(rangeBegin, rangeEnd) |
		   std::views::filter([&](auto &sLab) { return !sLab.isRMW(); }) |
		   std::views::transform([&](auto &sLab) {
			   auto *pLab = g.co_imm_pred(&sLab);
			   return pLab ? (EventLabel *)pLab : (EventLabel *)g.getInitLabel();
		   });
	std::ranges::copy(cos, std::back_inserter(result));
	result.push_back(rangeEnd == std::ranges::end(g.co(wLab->getAddr()))
				 ? g.co_max(wLab->getAddr())
				 : (!g.co_imm_pred(&*rangeEnd)
					    ? (EventLabel *)g.getInitLabel()
					    : (EventLabel *)g.co_imm_pred(&*rangeEnd)));
	return result;
}
bool LKMMChecker::visitCoherenceIterative(std::vector<DFSWorklistEntry> &worklist,
					  const EventLabel *initLab) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		case 0: {
			if (isFinishing) {
				break;
			}

			if (lab == initLab)
				return false;
			[[maybe_unused]] auto &g = *lab->getParent();

			break;
		}
		case 1: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedCoherence_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					}
				}

			break;
		}
		case 2: {
			if (isFinishing) {
				visitedCoherence_2.setStatus(lab->getStamp().get(),
							     NodeStatus::left);
				break;
			}

			auto status = visitedCoherence_2.getStatus(lab->getStamp().get());
			if (status != NodeStatus::unseen)
				break; /* already explored */

			worklist.emplace_back(2, lab, true);
			visitedCoherence_2.setStatus(lab->getStamp().get(), NodeStatus::entered);

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					worklist.emplace_back(0, pLab);
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				worklist.emplace_back(0, pLab);
			}
			if (auto pLab = g.tc_pred(lab); pLab) {
				worklist.emplace_back(0, pLab);
			}
			if (auto pLab = g.tj_pred(lab); pLab) {
				worklist.emplace_back(0, pLab);
			}
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedCoherence_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status = visitedCoherence_2.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(2, pLab);
				}
			}
			if (auto pLab = g.tc_pred(lab); pLab) {
				auto status = visitedCoherence_2.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(2, pLab);
				}
			}
			if (auto pLab = g.tj_pred(lab); pLab) {
				auto status = visitedCoherence_2.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(2, pLab);
				}
			}

			break;
		}
		case 3: {
			if (isFinishing) {
				visitedCoherence_3.setStatus(lab->getStamp().get(),
							     NodeStatus::left);
				break;
			}

			auto status = visitedCoherence_3.getStatus(lab->getStamp().get());
			if (status != NodeStatus::unseen)
				break; /* already explored */

			worklist.emplace_back(3, lab, true);
			visitedCoherence_3.setStatus(lab->getStamp().get(), NodeStatus::entered);

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.fr_imm_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedCoherence_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					}
				}
			if (auto pLab = g.co_imm_pred(lab); pLab) {
				auto status = visitedCoherence_2.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(2, pLab);
				}
			}
			if (auto pLab = g.rf_pred(lab); pLab) {
				auto status = visitedCoherence_2.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(2, pLab);
				}
			}
			for (auto &tmp : g.fr_imm_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedCoherence_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					}
				}
			if (auto pLab = g.co_imm_pred(lab); pLab) {
				auto status = visitedCoherence_3.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(3, pLab);
				}
			}
			if (auto pLab = g.rf_pred(lab); pLab) {
				auto status = visitedCoherence_3.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(3, pLab);
				}
			}

			break;
		}
		case 4: {
			if (isFinishing) {
				visitedCoherence_4.setStatus(lab->getStamp().get(),
							     NodeStatus::left);
				break;
			}

			auto status = visitedCoherence_4.getStatus(lab->getStamp().get());
			if (status != NodeStatus::unseen)
				break; /* already explored */

			worklist.emplace_back(4, lab, true);
			visitedCoherence_4.setStatus(lab->getStamp().get(), NodeStatus::entered);

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedCoherence_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status = visitedCoherence_3.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(3, pLab);
				}
			}
			if (auto pLab = g.tc_pred(lab); pLab) {
				auto status = visitedCoherence_3.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(3, pLab);
				}
			}
			if (auto pLab = g.tj_pred(lab); pLab) {
				auto status = visitedCoherence_3.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(3, pLab);
				}
			}
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedCoherence_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status = visitedCoherence_4.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(4, pLab);
				}
			}
			if (auto pLab = g.tc_pred(lab); pLab) {
				auto status = visitedCoherence_4.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(4, pLab);
				}
			}
			if (auto pLab = g.tj_pred(lab); pLab) {
				auto status = visitedCoherence_4.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(4, pLab);
				}
			}

			break;
		}
		default:
			UNREACHABLE();
		}
	}
	return true;
}

bool LKMMChecker::visitCoherenceRelinche(const ExecutionGraph &g) const
{
	for (auto &lab : g.labels()) {
		if (!genmc::isa<MethodBeginLabel>(&lab))
			continue;

		visitedCoherence_2.maybeClearResize(g.getMaxStamp().get() + 1);
		visitedCoherence_3.maybeClearResize(g.getMaxStamp().get() + 1);
		visitedCoherence_4.maybeClearResize(g.getMaxStamp().get() + 1);

		/* Explore from this accepting state using DFS */
		std::vector<DFSWorklistEntry> startState = {{1, &lab}};
		if (!visitCoherenceIterative(startState, &lab /* initLab */))
			return false;
	}
	return true;
}

bool LKMMChecker::visitConsAcyclic1Iterative(std::vector<DFSWorklistEntry> &worklist) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		case 0: {
			if (isFinishing) {
				visitedConsAcyclic1_0.set(lab->getStamp().get(),
							  visitedConsAcyclic1Accepting,
							  NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic1_0.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1_0.set(lab->getStamp().get(),
							  visitedConsAcyclic1Accepting,
							  NodeStatus::entered);
				worklist.emplace_back(0, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic1Accepting >
					    visitedConsAcyclic1_0.getCount(lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic1_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic1_23.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(23, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_23.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}

			break;
		}
		case 1: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic1_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_0.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 2: {
			if (isFinishing) {
				visitedConsAcyclic1_2.set(lab->getStamp().get(),
							  visitedConsAcyclic1Accepting,
							  NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic1_2.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1_2.set(lab->getStamp().get(),
							  visitedConsAcyclic1Accepting,
							  NodeStatus::entered);
				worklist.emplace_back(2, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic1Accepting >
					    visitedConsAcyclic1_2.getCount(lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rfi_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_0.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(0, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic1Accepting >
						    visitedConsAcyclic1_0.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.rfi_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					worklist.emplace_back(1, pLab);
				}

			break;
		}
		case 3: {
			if (isFinishing) {
				visitedConsAcyclic1_3.set(lab->getStamp().get(),
							  visitedConsAcyclic1Accepting,
							  NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic1_3.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1_3.set(lab->getStamp().get(),
							  visitedConsAcyclic1Accepting,
							  NodeStatus::entered);
				worklist.emplace_back(3, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic1Accepting >
					    visitedConsAcyclic1_3.getCount(lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic1_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic1_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic1_23.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(23, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_23.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic1_23.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(23, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_23.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}

			break;
		}
		case 4: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic1_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_3.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 5: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic1_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.ctrl_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic1_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic1_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic1_23.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(23, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_23.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.ctrl_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic1_23.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(23, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_23.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic1_23.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(23, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_23.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}

			break;
		}
		case 6: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic1_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic1_23.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(23, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_23.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}

			break;
		}
		case 7: {
			if (isFinishing) {
				visitedConsAcyclic1_7.set(lab->getStamp().get(),
							  visitedConsAcyclic1Accepting,
							  NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic1_7.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1_7.set(lab->getStamp().get(),
							  visitedConsAcyclic1Accepting,
							  NodeStatus::entered);
				worklist.emplace_back(7, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic1Accepting >
					    visitedConsAcyclic1_7.getCount(lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isNotAtomic() && genmc::isa<WriteLabel>(pLab)) {
					worklist.emplace_back(6, pLab);
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_7.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(7, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic1Accepting >
						    visitedConsAcyclic1_7.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic1_23.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(23, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_23.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 8: {
			if (isFinishing) {
				visitedConsAcyclic1_8.set(lab->getStamp().get(),
							  visitedConsAcyclic1Accepting,
							  NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic1_8.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1_8.set(lab->getStamp().get(),
							  visitedConsAcyclic1Accepting,
							  NodeStatus::entered);
				worklist.emplace_back(8, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic1Accepting >
					    visitedConsAcyclic1_8.getCount(lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
				    static_cast<const LkmmFenceLabel *>(
					    static_cast<const EventLabel *>(pLab))
						    ->getFenceKind() == LkmmFenceKind::Wmb) {
					auto status = visitedConsAcyclic1_7.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(7, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_7.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_8.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(8, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic1Accepting >
						    visitedConsAcyclic1_8.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 9: {
			if (isFinishing) {
				visitedConsAcyclic1_9.set(lab->getStamp().get(),
							  visitedConsAcyclic1Accepting,
							  NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic1_9.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1_9.set(lab->getStamp().get(),
							  visitedConsAcyclic1Accepting,
							  NodeStatus::entered);
				worklist.emplace_back(9, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic1Accepting >
					    visitedConsAcyclic1_9.getCount(lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_9.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(9, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic1Accepting >
						    visitedConsAcyclic1_9.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic1_23.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(23, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_23.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic1_23.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(23, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_23.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 10: {
			if (isFinishing) {
				visitedConsAcyclic1_10.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic1_10.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1_10.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(10, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic1Accepting > visitedConsAcyclic1_10.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
				    static_cast<const LkmmFenceLabel *>(
					    static_cast<const EventLabel *>(pLab))
						    ->getFenceKind() == LkmmFenceKind::Mb) {
					auto status = visitedConsAcyclic1_9.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(9, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_9.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_10.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(10, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic1Accepting >
						    visitedConsAcyclic1_10.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic1_11.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(11, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_11.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
				    static_cast<const LkmmFenceLabel *>(
					    static_cast<const EventLabel *>(pLab))
						    ->getFenceKind() ==
					    LkmmFenceKind::AfterAtomic) {
					auto status = visitedConsAcyclic1_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_12.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
				    static_cast<const LkmmFenceLabel *>(
					    static_cast<const EventLabel *>(pLab))
						    ->getFenceKind() ==
					    LkmmFenceKind::AfterSpinlock) {
					auto status = visitedConsAcyclic1_13.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(13, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_13.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
				    static_cast<const LkmmFenceLabel *>(
					    static_cast<const EventLabel *>(pLab))
						    ->getFenceKind() ==
					    LkmmFenceKind::AfterUnlockLock) {
					auto status = visitedConsAcyclic1_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_16.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic1_23.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(23, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_23.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 11: {
			if (isFinishing) {
				visitedConsAcyclic1_11.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic1_11.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1_11.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(11, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic1Accepting > visitedConsAcyclic1_11.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
				    static_cast<const LkmmFenceLabel *>(
					    static_cast<const EventLabel *>(pLab))
						    ->getFenceKind() ==
					    LkmmFenceKind::BeforeAtomic) {
					auto status = visitedConsAcyclic1_9.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(9, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_9.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_11.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(11, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic1Accepting >
						    visitedConsAcyclic1_11.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 12: {
			if (isFinishing) {
				visitedConsAcyclic1_12.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic1_12.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1_12.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(12, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic1Accepting > visitedConsAcyclic1_12.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic1_9.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(9, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_9.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_12.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(12, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic1Accepting >
						    visitedConsAcyclic1_12.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic1_23.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(23, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_23.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic1_23.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(23, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_23.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 13: {
			if (isFinishing) {
				visitedConsAcyclic1_13.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic1_13.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1_13.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(13, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic1Accepting > visitedConsAcyclic1_13.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab)) {
					auto status = visitedConsAcyclic1_9.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(9, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_9.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_13.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(13, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic1Accepting >
						    visitedConsAcyclic1_13.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic1_23.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(23, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_23.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic1_23.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(23, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_23.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 14: {
			if (isFinishing) {
				visitedConsAcyclic1_14.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic1_14.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1_14.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(14, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic1Accepting > visitedConsAcyclic1_14.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<UnlockWriteLabel>(pLab)) {
					auto status = visitedConsAcyclic1_9.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(9, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_9.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_14.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(14, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic1Accepting >
						    visitedConsAcyclic1_14.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 15: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && genmc::isa<UnlockWriteLabel>(pLab)) {
					auto status = visitedConsAcyclic1_9.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(9, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_9.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfi_pred(lab); pLab)
				if (true && genmc::isa<UnlockWriteLabel>(pLab)) {
					auto status = visitedConsAcyclic1_9.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(9, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_9.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 16: {
			if (isFinishing) {
				visitedConsAcyclic1_16.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic1_16.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1_16.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(16, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic1Accepting > visitedConsAcyclic1_16.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true &&
				    (genmc::isa<LockCasReadLabel>(pLab) ||
				     genmc::isa<TrylockCasReadLabel>(pLab)) &&
				    static_cast<const ReadLabel *>(
					    static_cast<const EventLabel *>(pLab))
					    ->isRMW()) {
					auto status = visitedConsAcyclic1_14.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(14, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_14.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true &&
				    (genmc::isa<LockCasReadLabel>(pLab) ||
				     genmc::isa<TrylockCasReadLabel>(pLab)) &&
				    static_cast<const ReadLabel *>(
					    static_cast<const EventLabel *>(pLab))
					    ->isRMW()) {
					worklist.emplace_back(15, pLab);
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_16.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(16, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic1Accepting >
						    visitedConsAcyclic1_16.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 17: {
			if (isFinishing) {
				visitedConsAcyclic1_17.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic1_17.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1_17.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(17, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic1Accepting > visitedConsAcyclic1_17.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_17.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(17, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic1Accepting >
						    visitedConsAcyclic1_17.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic1_23.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(23, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_23.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 18: {
			if (isFinishing) {
				visitedConsAcyclic1_18.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic1_18.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1_18.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(18, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic1Accepting > visitedConsAcyclic1_18.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_18.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(18, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic1Accepting >
						    visitedConsAcyclic1_18.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true &&
				    (genmc::isa<LockCasReadLabel>(pLab) ||
				     genmc::isa<TrylockCasReadLabel>(pLab)) &&
				    static_cast<const ReadLabel *>(
					    static_cast<const EventLabel *>(pLab))
					    ->isRMW()) {
					auto status = visitedConsAcyclic1_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true &&
				    (genmc::isa<LockCasReadLabel>(pLab) ||
				     genmc::isa<TrylockCasReadLabel>(pLab)) &&
				    static_cast<const ReadLabel *>(
					    static_cast<const EventLabel *>(pLab))
					    ->isRMW()) {
					auto status = visitedConsAcyclic1_22.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(22, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_22.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    pLab->isAtLeastAcquire() &&
				    (!genmc::isa<CasReadLabel>(pLab) ||
				     static_cast<const ReadLabel *>(
					     static_cast<const EventLabel *>(pLab))
					     ->isRMW()) &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic1_23.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(23, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_23.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 19: {
			if (isFinishing) {
				visitedConsAcyclic1_19.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic1_19.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1_19.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(19, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic1Accepting > visitedConsAcyclic1_19.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_19.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(19, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic1Accepting >
						    visitedConsAcyclic1_19.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic1_23.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(23, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_23.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 20: {
			if (isFinishing) {
				visitedConsAcyclic1_20.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic1_20.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1_20.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(20, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic1Accepting > visitedConsAcyclic1_20.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
				    static_cast<const LkmmFenceLabel *>(
					    static_cast<const EventLabel *>(pLab))
						    ->getFenceKind() == LkmmFenceKind::Rmb) {
					auto status = visitedConsAcyclic1_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_20.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(20, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic1Accepting >
						    visitedConsAcyclic1_20.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 21: {
			if (isFinishing) {
				visitedConsAcyclic1_21.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic1_21.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1_21.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(21, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic1Accepting > visitedConsAcyclic1_21.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<UnlockWriteLabel>(pLab)) {
					auto status = visitedConsAcyclic1_17.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(17, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_17.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_21.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(21, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic1Accepting >
						    visitedConsAcyclic1_21.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 22: {
			if (isFinishing) {
				visitedConsAcyclic1_22.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic1_22.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1_22.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(22, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic1Accepting > visitedConsAcyclic1_22.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rfi_pred(lab); pLab)
				if (true && genmc::isa<UnlockWriteLabel>(pLab)) {
					auto status = visitedConsAcyclic1_17.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(17, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_17.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 23: {
			if (isFinishing) {
				--visitedConsAcyclic1Accepting;
				visitedConsAcyclic1_23.set(lab->getStamp().get(),
							   visitedConsAcyclic1Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic1_23.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				++visitedConsAcyclic1Accepting;
				visitedConsAcyclic1_23.setIncr(lab->getStamp().get(),
							       visitedConsAcyclic1Accepting,
							       NodeStatus::entered);
				worklist.emplace_back(23, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic1Accepting > visitedConsAcyclic1_23.getCount(
									   lab->getStamp().get()) ||
				    1)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true) {
						auto status = visitedConsAcyclic1_2.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(2, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_2.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true) {
						auto status = visitedConsAcyclic1_2.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(2, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_2.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.ctrl_preds(lab))
					if (auto *pLab = &tmp; true) {
						auto status = visitedConsAcyclic1_2.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(2, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_2.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.data_preds(lab))
					if (auto *pLab = &tmp; true) {
						auto status = visitedConsAcyclic1_2.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(2, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_2.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.rfi_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic1_3.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(3, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_3.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.rfi_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						worklist.emplace_back(4, pLab);
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab) &&
			    ((genmc::isa<ReadLabel>(lab) &&
			      genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
			     (genmc::isa<WriteLabel>(lab) &&
			      genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						worklist.emplace_back(5, pLab);
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() ==
						    LkmmFenceKind::Wmb) {
						auto status = visitedConsAcyclic1_7.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(7, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_7.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic1_8.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(8, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_8.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab) &&
			    ((genmc::isa<ReadLabel>(lab) &&
			      genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
			     (genmc::isa<WriteLabel>(lab) &&
			      genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() ==
						    LkmmFenceKind::BeforeAtomic) {
						auto status = visitedConsAcyclic1_9.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(9, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_9.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() == LkmmFenceKind::Mb) {
						auto status = visitedConsAcyclic1_9.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(9, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_9.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab) &&
			    ((genmc::isa<ReadLabel>(lab) &&
			      genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
			     (genmc::isa<WriteLabel>(lab) &&
			      genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() ==
						    LkmmFenceKind::BeforeAtomic) {
						auto status = visitedConsAcyclic1_9.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(9, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_9.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() == LkmmFenceKind::Mb) {
						auto status = visitedConsAcyclic1_9.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(9, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_9.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic1_9.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(9, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_9.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic1_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_10.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic1_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_10.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab) &&
			    ((genmc::isa<ReadLabel>(lab) &&
			      genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
			     (genmc::isa<WriteLabel>(lab) &&
			      genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic1_11.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(11, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_11.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic1_11.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(11, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_11.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab) &&
			    ((genmc::isa<ReadLabel>(lab) &&
			      genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
			     (genmc::isa<WriteLabel>(lab) &&
			      genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic1_11.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(11, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_11.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic1_11.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(11, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_11.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() ==
						    LkmmFenceKind::AfterAtomic) {
						auto status = visitedConsAcyclic1_12.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(12, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_12.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() ==
						    LkmmFenceKind::AfterAtomic) {
						auto status = visitedConsAcyclic1_12.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(12, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_12.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() ==
						    LkmmFenceKind::AfterSpinlock) {
						auto status = visitedConsAcyclic1_13.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(13, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_13.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() ==
						    LkmmFenceKind::AfterSpinlock) {
						auto status = visitedConsAcyclic1_13.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(13, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_13.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() ==
						    LkmmFenceKind::AfterUnlockLock) {
						auto status = visitedConsAcyclic1_16.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(16, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_16.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() ==
						    LkmmFenceKind::AfterUnlockLock) {
						auto status = visitedConsAcyclic1_16.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(16, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_16.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab) && lab->isAtLeastRelease() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic1_17.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(17, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_17.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic1_18.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(18, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_18.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (true && genmc::isa<ReadLabel>(lab) && !lab->isLkmmNoreturn() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() ==
						    LkmmFenceKind::Rmb) {
						auto status = visitedConsAcyclic1_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && !lab->isLkmmNoreturn() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic1_20.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(20, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic1Accepting >
							    visitedConsAcyclic1_20.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true &&
					    (genmc::isa<LockCasReadLabel>(pLab) ||
					     genmc::isa<TrylockCasReadLabel>(pLab)) &&
					    static_cast<const ReadLabel *>(
						    static_cast<const EventLabel *>(pLab))
						    ->isRMW()) {
						auto status = visitedConsAcyclic1_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true &&
					    (genmc::isa<LockCasReadLabel>(pLab) ||
					     genmc::isa<TrylockCasReadLabel>(pLab)) &&
					    static_cast<const ReadLabel *>(
						    static_cast<const EventLabel *>(pLab))
						    ->isRMW()) {
						auto status = visitedConsAcyclic1_22.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(22, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_22.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic1_23.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(23, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic1Accepting >
									 visitedConsAcyclic1_23.getCount(
										 pLab->getStamp()
											 .get()) ||
								 1)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic1_23.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(23, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_23.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic1_23.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(23, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic1Accepting >
									 visitedConsAcyclic1_23.getCount(
										 pLab->getStamp()
											 .get()) ||
								 1)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.ctrl_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic1_23.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(23, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic1Accepting >
									 visitedConsAcyclic1_23.getCount(
										 pLab->getStamp()
											 .get()) ||
								 1)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.data_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic1_23.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(23, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic1Accepting >
									 visitedConsAcyclic1_23.getCount(
										 pLab->getStamp()
											 .get()) ||
								 1)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic1_23.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(23, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_23.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				for (auto &tmp : g.fri_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic1_23.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(23, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic1Accepting >
									 visitedConsAcyclic1_23.getCount(
										 pLab->getStamp()
											 .get()) ||
								 1)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()))
				for (auto &tmp : g.moi_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic1_23.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(23, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic1Accepting >
									 visitedConsAcyclic1_23.getCount(
										 pLab->getStamp()
											 .get()) ||
								 1)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()))
				for (auto *pLab : g.prop_int_preds(lab))
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic1_23.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(23, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_23.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isAtLeastAcquire() &&
					    (!genmc::isa<CasReadLabel>(pLab) ||
					     static_cast<const ReadLabel *>(
						     static_cast<const EventLabel *>(pLab))
						     ->isRMW()) &&
					    !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic1_23.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(23, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_23.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.rfe_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic1_23.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(23, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_23.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.tc_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic1_23.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(23, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_23.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.tj_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic1_23.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(23, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_23.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic1_23.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(23, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_23.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic1_23.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(23, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_23.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab) && lab->isAtLeastRelease() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic1_23.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(23, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic1Accepting >
								    visitedConsAcyclic1_23.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}

			break;
		}
		default:
			UNREACHABLE();
		}
	}
	return true;
}

bool LKMMChecker::visitConsAcyclic1(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedConsAcyclic1Accepting = 0;
	visitedConsAcyclic1_0.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_2.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_3.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_7.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_8.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_9.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_10.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_11.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_12.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_13.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_14.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_16.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_17.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_18.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_19.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_20.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_21.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_22.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_23.maybeClearResize(g.getMaxStamp().get() + 1);

	/* States we need to explore from using DFS */
	std::vector<DFSWorklistEntry> startStates = {
		{23, lab},
	};

	return visitConsAcyclic1Iterative(startStates);
}

bool LKMMChecker::visitConsAcyclic1Full(const ExecutionGraph &g) const
{
	visitedConsAcyclic1Accepting = 0;
	visitedConsAcyclic1_0.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_2.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_3.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_7.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_8.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_9.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_10.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_11.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_12.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_13.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_14.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_16.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_17.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_18.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_19.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_20.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_21.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_22.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_23.maybeClearResize(g.getMaxStamp().get() + 1);

	auto exploreLab = [&](auto &lab) {
		/* Explore from all accepting states using DFS */
		std::vector<DFSWorklistEntry> startStates = {
			{23, &lab},
		};

		return visitConsAcyclic1Iterative(startStates);
	};

	return std::ranges::all_of(g.labels(), exploreLab);
}

bool LKMMChecker::checkConsAcyclic1(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	return visitConsAcyclic1(lab);
}

bool LKMMChecker::checkConsAcyclic1(const ExecutionGraph &g) const
{
	return visitConsAcyclic1Full(g);
}

bool LKMMChecker::visitConsAcyclic2Iterative(std::vector<DFSWorklistEntry> &worklist) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		case 0: {
			if (isFinishing) {
				visitedConsAcyclic2_0.set(lab->getStamp().get(),
							  visitedConsAcyclic2Accepting,
							  NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_0.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_0.set(lab->getStamp().get(),
							  visitedConsAcyclic2Accepting,
							  NodeStatus::entered);
				worklist.emplace_back(0, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting >
					    visitedConsAcyclic2_0.getCount(lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.fre_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_44.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(44, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_44.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.moe_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_44.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(44, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_44.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}

			break;
		}
		case 1: {
			if (isFinishing) {
				visitedConsAcyclic2_1.set(lab->getStamp().get(),
							  visitedConsAcyclic2Accepting,
							  NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_1.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_1.set(lab->getStamp().get(),
							  visitedConsAcyclic2Accepting,
							  NodeStatus::entered);
				worklist.emplace_back(1, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting >
					    visitedConsAcyclic2_1.getCount(lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_0.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_3.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_4.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_10.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_12.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_16.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_18.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(18, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_18.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_44.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(44, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_44.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 2: {
			if (isFinishing) {
				visitedConsAcyclic2_2.set(lab->getStamp().get(),
							  visitedConsAcyclic2Accepting,
							  NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_2.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_2.set(lab->getStamp().get(),
							  visitedConsAcyclic2Accepting,
							  NodeStatus::entered);
				worklist.emplace_back(2, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting >
					    visitedConsAcyclic2_2.getCount(lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_0.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_0.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_1.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_1.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_2.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(2, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_2.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_3.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_3.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_4.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_4.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
				    genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_10.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_10.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_12.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_16.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_16.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_18.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(18, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_18.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_44.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(44, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_44.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_44.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(44, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_44.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 3: {
			if (isFinishing) {
				visitedConsAcyclic2_3.set(lab->getStamp().get(),
							  visitedConsAcyclic2Accepting,
							  NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_3.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_3.set(lab->getStamp().get(),
							  visitedConsAcyclic2Accepting,
							  NodeStatus::entered);
				worklist.emplace_back(3, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting >
					    visitedConsAcyclic2_3.getCount(lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_0.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_1.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
				    static_cast<const LkmmFenceLabel *>(
					    static_cast<const EventLabel *>(pLab))
						    ->getFenceKind() == LkmmFenceKind::Mb) {
					auto status = visitedConsAcyclic2_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_3.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(3, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_3.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_3.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_3.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_4.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_4.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_4.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
				    static_cast<const LkmmFenceLabel *>(
					    static_cast<const EventLabel *>(pLab))
						    ->getFenceKind() ==
					    LkmmFenceKind::AfterAtomic) {
					auto status = visitedConsAcyclic2_5.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(5, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_5.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
				    static_cast<const LkmmFenceLabel *>(
					    static_cast<const EventLabel *>(pLab))
						    ->getFenceKind() ==
					    LkmmFenceKind::AfterSpinlock) {
					auto status = visitedConsAcyclic2_6.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_6.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
				    static_cast<const LkmmFenceLabel *>(
					    static_cast<const EventLabel *>(pLab))
						    ->getFenceKind() ==
					    LkmmFenceKind::AfterUnlockLock) {
					auto status = visitedConsAcyclic2_9.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(9, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_9.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_10.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_12.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_16.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_18.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(18, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_18.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_44.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(44, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_44.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 4: {
			if (isFinishing) {
				visitedConsAcyclic2_4.set(lab->getStamp().get(),
							  visitedConsAcyclic2Accepting,
							  NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_4.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_4.set(lab->getStamp().get(),
							  visitedConsAcyclic2Accepting,
							  NodeStatus::entered);
				worklist.emplace_back(4, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting >
					    visitedConsAcyclic2_4.getCount(lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
				    static_cast<const LkmmFenceLabel *>(
					    static_cast<const EventLabel *>(pLab))
						    ->getFenceKind() ==
					    LkmmFenceKind::BeforeAtomic) {
					auto status = visitedConsAcyclic2_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_4.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(4, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_4.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 5: {
			if (isFinishing) {
				visitedConsAcyclic2_5.set(lab->getStamp().get(),
							  visitedConsAcyclic2Accepting,
							  NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_5.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_5.set(lab->getStamp().get(),
							  visitedConsAcyclic2Accepting,
							  NodeStatus::entered);
				worklist.emplace_back(5, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting >
					    visitedConsAcyclic2_5.getCount(lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_0.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_0.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_1.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_1.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_3.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_3.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_4.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_4.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_5.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(5, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_5.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
				    genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_10.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_10.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_12.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_16.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_16.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_18.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(18, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_18.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_44.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(44, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_44.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_44.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(44, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_44.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 6: {
			if (isFinishing) {
				visitedConsAcyclic2_6.set(lab->getStamp().get(),
							  visitedConsAcyclic2Accepting,
							  NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_6.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_6.set(lab->getStamp().get(),
							  visitedConsAcyclic2Accepting,
							  NodeStatus::entered);
				worklist.emplace_back(6, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting >
					    visitedConsAcyclic2_6.getCount(lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_0.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_0.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_1.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_1.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
					    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_3.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_3.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_4.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_4.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_6.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(6, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_6.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_10.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_10.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_12.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_16.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_16.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_18.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(18, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_18.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_44.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(44, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_44.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_44.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(44, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_44.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 7: {
			if (isFinishing) {
				visitedConsAcyclic2_7.set(lab->getStamp().get(),
							  visitedConsAcyclic2Accepting,
							  NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_7.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_7.set(lab->getStamp().get(),
							  visitedConsAcyclic2Accepting,
							  NodeStatus::entered);
				worklist.emplace_back(7, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting >
					    visitedConsAcyclic2_7.getCount(lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<UnlockWriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_7.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(7, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_7.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 8: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && genmc::isa<UnlockWriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfi_pred(lab); pLab)
				if (true && genmc::isa<UnlockWriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 9: {
			if (isFinishing) {
				visitedConsAcyclic2_9.set(lab->getStamp().get(),
							  visitedConsAcyclic2Accepting,
							  NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_9.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_9.set(lab->getStamp().get(),
							  visitedConsAcyclic2Accepting,
							  NodeStatus::entered);
				worklist.emplace_back(9, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting >
					    visitedConsAcyclic2_9.getCount(lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true &&
				    (genmc::isa<LockCasReadLabel>(pLab) ||
				     genmc::isa<TrylockCasReadLabel>(pLab)) &&
				    static_cast<const ReadLabel *>(
					    static_cast<const EventLabel *>(pLab))
					    ->isRMW()) {
					auto status = visitedConsAcyclic2_7.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(7, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_7.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true &&
				    (genmc::isa<LockCasReadLabel>(pLab) ||
				     genmc::isa<TrylockCasReadLabel>(pLab)) &&
				    static_cast<const ReadLabel *>(
					    static_cast<const EventLabel *>(pLab))
					    ->isRMW()) {
					worklist.emplace_back(8, pLab);
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_9.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(9, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_9.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 10: {
			if (isFinishing) {
				visitedConsAcyclic2_10.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_10.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_10.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(10, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_10.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_0.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_1.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_3.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_3.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_4.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_4.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_10.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(10, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_10.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_10.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_12.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_16.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_18.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(18, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_18.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_44.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(44, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_44.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 11: {
			if (isFinishing) {
				visitedConsAcyclic2_11.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_11.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_11.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(11, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_11.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_0.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_3.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_4.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_10.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_11.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(11, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_11.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_12.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_16.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_18.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(18, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_18.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_44.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(44, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_44.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 12: {
			if (isFinishing) {
				visitedConsAcyclic2_12.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_12.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_12.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(12, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_12.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
				    static_cast<const LkmmFenceLabel *>(
					    static_cast<const EventLabel *>(pLab))
						    ->getFenceKind() == LkmmFenceKind::Wmb) {
					auto status = visitedConsAcyclic2_11.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(11, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_11.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_12.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(12, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_12.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 13: {
			if (isFinishing) {
				visitedConsAcyclic2_13.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_13.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_13.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(13, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_13.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_0.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_3.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_3.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_4.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_4.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_10.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_12.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_13.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(13, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_13.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_16.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_18.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(18, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_18.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_44.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(44, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_44.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 14: {
			if (isFinishing) {
				visitedConsAcyclic2_14.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_14.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_14.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(14, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_14.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<UnlockWriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_13.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(13, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_13.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_14.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(14, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_14.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 15: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && genmc::isa<UnlockWriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_13.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(13, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_13.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfi_pred(lab); pLab)
				if (true && genmc::isa<UnlockWriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_13.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(13, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_13.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 16: {
			if (isFinishing) {
				visitedConsAcyclic2_16.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_16.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_16.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(16, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_16.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true &&
				    (genmc::isa<LockCasReadLabel>(pLab) ||
				     genmc::isa<TrylockCasReadLabel>(pLab)) &&
				    static_cast<const ReadLabel *>(
					    static_cast<const EventLabel *>(pLab))
					    ->isRMW()) {
					auto status = visitedConsAcyclic2_14.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(14, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_14.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true &&
				    (genmc::isa<LockCasReadLabel>(pLab) ||
				     genmc::isa<TrylockCasReadLabel>(pLab)) &&
				    static_cast<const ReadLabel *>(
					    static_cast<const EventLabel *>(pLab))
					    ->isRMW()) {
					worklist.emplace_back(15, pLab);
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_16.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(16, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_16.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 17: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_2.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_3.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_4.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_10.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_12.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_16.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_18.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(18, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_18.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 18: {
			if (isFinishing) {
				visitedConsAcyclic2_18.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_18.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_18.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(18, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_18.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					worklist.emplace_back(17, pLab);
				}

			break;
		}
		case 19: {
			if (isFinishing) {
				visitedConsAcyclic2_19.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_19.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_19.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(19, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_19.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_0.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_0.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_1.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_1.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
				    genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_10.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_10.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_12.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_16.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_16.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_18.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(18, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_18.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_19.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(19, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_19.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_20.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(20, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_20.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_20.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(20, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_20.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_30.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(30, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_30.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_33.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(33, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_33.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_33.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(33, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_33.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_34.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(34, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_34.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_35.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(35, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_35.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_37.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(37, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_37.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
				    genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_38.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(38, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_38.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_38.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(38, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_38.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_39.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(39, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_39.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_39.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(39, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_39.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic()) &&
				    genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_41.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(41, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_41.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_41.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(41, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_41.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_44.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(44, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_44.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_44.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(44, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_44.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 20: {
			if (isFinishing) {
				visitedConsAcyclic2_20.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_20.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_20.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(20, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_20.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_0.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_1.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_10.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_12.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_16.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_18.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(18, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_18.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
				    static_cast<const LkmmFenceLabel *>(
					    static_cast<const EventLabel *>(pLab))
						    ->getFenceKind() == LkmmFenceKind::Mb) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_20.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(20, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_20.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_20.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(20, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_20.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_20.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(20, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_20.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
				    static_cast<const LkmmFenceLabel *>(
					    static_cast<const EventLabel *>(pLab))
						    ->getFenceKind() ==
					    LkmmFenceKind::AfterAtomic) {
					auto status = visitedConsAcyclic2_22.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(22, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_22.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
				    static_cast<const LkmmFenceLabel *>(
					    static_cast<const EventLabel *>(pLab))
						    ->getFenceKind() ==
					    LkmmFenceKind::AfterSpinlock) {
					auto status = visitedConsAcyclic2_23.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(23, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_23.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
				    static_cast<const LkmmFenceLabel *>(
					    static_cast<const EventLabel *>(pLab))
						    ->getFenceKind() ==
					    LkmmFenceKind::AfterUnlockLock) {
					auto status = visitedConsAcyclic2_26.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(26, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_26.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_30.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(30, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_30.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_33.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(33, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_33.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_34.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(34, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_34.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_35.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(35, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_35.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_37.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(37, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_37.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_38.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(38, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_38.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_39.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(39, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_39.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_41.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(41, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_41.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_44.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(44, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_44.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 21: {
			if (isFinishing) {
				visitedConsAcyclic2_21.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_21.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_21.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(21, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_21.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
				    static_cast<const LkmmFenceLabel *>(
					    static_cast<const EventLabel *>(pLab))
						    ->getFenceKind() ==
					    LkmmFenceKind::BeforeAtomic) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_21.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(21, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_21.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 22: {
			if (isFinishing) {
				visitedConsAcyclic2_22.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_22.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_22.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(22, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_22.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_0.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_0.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_1.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_1.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
				    genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_10.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_10.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_12.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_16.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_16.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_18.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(18, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_18.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_20.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(20, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_20.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_20.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(20, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_20.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_22.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(22, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_22.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_30.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(30, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_30.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_33.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(33, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_33.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_33.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(33, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_33.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_34.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(34, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_34.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_35.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(35, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_35.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_37.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(37, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_37.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
				    genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_38.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(38, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_38.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_38.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(38, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_38.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_39.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(39, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_39.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_39.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(39, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_39.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic()) &&
				    genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_41.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(41, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_41.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_41.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(41, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_41.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_44.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(44, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_44.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_44.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(44, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_44.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 23: {
			if (isFinishing) {
				visitedConsAcyclic2_23.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_23.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_23.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(23, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_23.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_0.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_0.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_1.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_1.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_10.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_10.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_12.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_16.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_16.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_18.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(18, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_18.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
					    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_20.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(20, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_20.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_20.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(20, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_20.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_23.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(23, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_23.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_30.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(30, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_30.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_33.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(33, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_33.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_33.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(33, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_33.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_34.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(34, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_34.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_35.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(35, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_35.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_37.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(37, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_37.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_38.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(38, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_38.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_38.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(38, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_38.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_39.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(39, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_39.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_39.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(39, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_39.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    genmc::isa<ReadLabel>(pLab) &&
					    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_41.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(41, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_41.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    genmc::isa<ReadLabel>(pLab) &&
					    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_41.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(41, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_41.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_44.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(44, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_44.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
				    genmc::isa<TrylockCasWriteLabel>(pLab) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_44.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(44, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_44.getCount(
								    pLab->getStamp().get()) ||
						    1)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 24: {
			if (isFinishing) {
				visitedConsAcyclic2_24.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_24.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_24.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(24, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_24.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<UnlockWriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_24.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(24, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_24.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 25: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && genmc::isa<UnlockWriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfi_pred(lab); pLab)
				if (true && genmc::isa<UnlockWriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 26: {
			if (isFinishing) {
				visitedConsAcyclic2_26.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_26.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_26.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(26, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_26.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true &&
				    (genmc::isa<LockCasReadLabel>(pLab) ||
				     genmc::isa<TrylockCasReadLabel>(pLab)) &&
				    static_cast<const ReadLabel *>(
					    static_cast<const EventLabel *>(pLab))
					    ->isRMW()) {
					auto status = visitedConsAcyclic2_24.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(24, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_24.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true &&
				    (genmc::isa<LockCasReadLabel>(pLab) ||
				     genmc::isa<TrylockCasReadLabel>(pLab)) &&
				    static_cast<const ReadLabel *>(
					    static_cast<const EventLabel *>(pLab))
					    ->isRMW()) {
					worklist.emplace_back(25, pLab);
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_26.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(26, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_26.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 27: {
			if (isFinishing) {
				visitedConsAcyclic2_27.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_27.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_27.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(27, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_27.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic2_29.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(29, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_29.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_30.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(30, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_30.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_33.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(33, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_33.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_38.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(38, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_38.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_39.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(39, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_39.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_41.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(41, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_41.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}

			break;
		}
		case 28: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_27.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(27, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_27.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 29: {
			if (isFinishing) {
				visitedConsAcyclic2_29.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_29.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_29.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(29, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_29.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rfi_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_27.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(27, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_27.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.rfi_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					worklist.emplace_back(28, pLab);
				}

			break;
		}
		case 30: {
			if (isFinishing) {
				visitedConsAcyclic2_30.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_30.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_30.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(30, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_30.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic2_29.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(29, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_29.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_30.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(30, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_30.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_33.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(33, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_33.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_38.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(38, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_38.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_39.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(39, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_39.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_41.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(41, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_41.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}

			break;
		}
		case 31: {
			if (isFinishing) {
				visitedConsAcyclic2_31.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_31.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_31.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(31, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_31.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic2_29.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(29, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_29.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic2_29.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(29, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_29.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_30.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(30, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_30.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_30.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(30, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_30.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_33.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(33, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_33.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_33.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(33, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_33.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_38.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(38, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_38.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_38.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(38, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_38.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_39.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(39, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_39.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_39.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(39, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_39.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_41.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(41, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_41.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_41.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(41, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_41.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}

			break;
		}
		case 32: {
			if (isFinishing) {
				visitedConsAcyclic2_32.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_32.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_32.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(32, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_32.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_31.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(31, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_31.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 33: {
			if (isFinishing) {
				visitedConsAcyclic2_33.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_33.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_33.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(33, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_33.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.fri_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.moi_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto *pLab : g.prop_int_preds(lab))
				if (true && genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.tc_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.tj_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.fri_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.moi_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto *pLab : g.prop_int_preds(lab))
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_20.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(20, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_20.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto *pLab : g.prop_int_preds(lab))
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_20.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(20, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_20.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_20.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(20, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_20.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.fri_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.moi_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto *pLab : g.prop_int_preds(lab))
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto *pLab : g.prop_int_preds(lab))
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.fri_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_30.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(30, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_30.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto *pLab : g.prop_int_preds(lab))
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_30.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(30, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_30.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfi_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_31.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(31, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_31.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfi_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_32.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(32, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_32.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.fri_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_33.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(33, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_33.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.moi_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_33.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(33, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_33.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto *pLab : g.prop_int_preds(lab))
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_33.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(33, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_33.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_33.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(33, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_33.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.tc_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_33.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(33, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_33.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.tj_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_33.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(33, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_33.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.moi_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_34.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(34, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_34.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto *pLab : g.prop_int_preds(lab))
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_34.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(34, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_34.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_34.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(34, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_34.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.moi_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_35.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(35, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_35.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto *pLab : g.prop_int_preds(lab))
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_35.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(35, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_35.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_35.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(35, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_35.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.moi_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_37.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(37, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_37.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto *pLab : g.prop_int_preds(lab))
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_37.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(37, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_37.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_37.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(37, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_37.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.fri_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_38.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(38, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_38.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.moi_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_38.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(38, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_38.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto *pLab : g.prop_int_preds(lab))
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_38.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(38, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_38.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_38.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(38, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_38.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.tc_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_38.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(38, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_38.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.tj_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_38.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(38, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_38.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.fri_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_39.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(39, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_39.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.moi_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_39.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(39, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_39.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto *pLab : g.prop_int_preds(lab))
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_39.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(39, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_39.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_39.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(39, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_39.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.tc_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_39.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(39, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_39.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.tj_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_39.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(39, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_39.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.fri_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_41.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(41, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_41.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.moi_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_41.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(41, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_41.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto *pLab : g.prop_int_preds(lab))
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_41.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(41, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_41.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfe_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_41.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(41, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_41.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.tc_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_41.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(41, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_41.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.tj_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_41.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(41, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_41.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 34: {
			if (isFinishing) {
				visitedConsAcyclic2_34.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_34.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_34.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(34, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_34.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.ctrl_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.ctrl_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.ctrl_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic2_29.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(29, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_29.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.ctrl_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic2_29.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(29, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_29.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic2_29.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(29, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_29.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_30.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(30, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_30.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.ctrl_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_30.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(30, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_30.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_30.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(30, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_30.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_33.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(33, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_33.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.ctrl_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_33.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(33, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_33.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_33.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(33, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_33.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_38.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(38, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_38.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.ctrl_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_38.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(38, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_38.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_38.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(38, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_38.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_39.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(39, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_39.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.ctrl_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_39.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(39, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_39.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_39.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(39, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_39.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_41.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(41, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_41.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.ctrl_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_41.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(41, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_41.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_41.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(41, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_41.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}

			break;
		}
		case 35: {
			if (isFinishing) {
				visitedConsAcyclic2_35.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_35.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_35.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(35, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_35.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_34.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(34, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_34.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 36: {
			if (isFinishing) {
				visitedConsAcyclic2_36.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_36.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_36.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(36, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_36.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_20.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(20, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_20.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isNotAtomic() && genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_30.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(30, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_30.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_33.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(33, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_33.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_34.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(34, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_34.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_35.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(35, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_35.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_36.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(36, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_36.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_37.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(37, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_37.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_38.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(38, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_38.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_39.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(39, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_39.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_41.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(41, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_41.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 37: {
			if (isFinishing) {
				visitedConsAcyclic2_37.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_37.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_37.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(37, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_37.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
				    static_cast<const LkmmFenceLabel *>(
					    static_cast<const EventLabel *>(pLab))
						    ->getFenceKind() == LkmmFenceKind::Wmb) {
					auto status = visitedConsAcyclic2_36.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(36, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_36.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_37.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(37, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_37.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 38: {
			if (isFinishing) {
				visitedConsAcyclic2_38.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_38.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_38.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(38, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_38.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_20.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(20, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_20.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_20.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(20, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_20.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_30.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(30, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_30.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_33.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(33, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_33.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_34.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(34, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_34.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_35.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(35, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_35.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_37.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(37, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_37.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_38.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(38, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_38.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_38.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(38, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_38.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_39.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(39, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_39.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_41.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(41, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_41.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 39: {
			if (isFinishing) {
				visitedConsAcyclic2_39.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_39.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_39.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(39, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_39.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    pLab->isAtLeastAcquire() &&
				    (!genmc::isa<CasReadLabel>(pLab) ||
				     static_cast<const ReadLabel *>(
					     static_cast<const EventLabel *>(pLab))
					     ->isRMW()) &&
				    genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    pLab->isAtLeastAcquire() &&
				    (!genmc::isa<CasReadLabel>(pLab) ||
				     static_cast<const ReadLabel *>(
					     static_cast<const EventLabel *>(pLab))
					     ->isRMW()) &&
				    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_20.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(20, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_20.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    pLab->isAtLeastAcquire() &&
				    (!genmc::isa<CasReadLabel>(pLab) ||
				     static_cast<const ReadLabel *>(
					     static_cast<const EventLabel *>(pLab))
					     ->isRMW()) &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_20.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(20, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_20.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    pLab->isAtLeastAcquire() &&
				    (!genmc::isa<CasReadLabel>(pLab) ||
				     static_cast<const ReadLabel *>(
					     static_cast<const EventLabel *>(pLab))
					     ->isRMW()) &&
				    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    pLab->isAtLeastAcquire() &&
				    (!genmc::isa<CasReadLabel>(pLab) ||
				     static_cast<const ReadLabel *>(
					     static_cast<const EventLabel *>(pLab))
					     ->isRMW()) &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    pLab->isAtLeastAcquire() &&
				    (!genmc::isa<CasReadLabel>(pLab) ||
				     static_cast<const ReadLabel *>(
					     static_cast<const EventLabel *>(pLab))
					     ->isRMW()) &&
				    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_30.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(30, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_30.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    pLab->isAtLeastAcquire() &&
				    (!genmc::isa<CasReadLabel>(pLab) ||
				     static_cast<const ReadLabel *>(
					     static_cast<const EventLabel *>(pLab))
					     ->isRMW()) &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_33.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(33, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_33.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    pLab->isAtLeastAcquire() &&
				    (!genmc::isa<CasReadLabel>(pLab) ||
				     static_cast<const ReadLabel *>(
					     static_cast<const EventLabel *>(pLab))
					     ->isRMW()) &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_34.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(34, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_34.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    pLab->isAtLeastAcquire() &&
				    (!genmc::isa<CasReadLabel>(pLab) ||
				     static_cast<const ReadLabel *>(
					     static_cast<const EventLabel *>(pLab))
					     ->isRMW()) &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_35.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(35, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_35.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    pLab->isAtLeastAcquire() &&
				    (!genmc::isa<CasReadLabel>(pLab) ||
				     static_cast<const ReadLabel *>(
					     static_cast<const EventLabel *>(pLab))
					     ->isRMW()) &&
				    !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_37.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(37, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_37.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && genmc::isa<ReadLabel>(pLab) &&
				    pLab->isAtLeastAcquire() &&
				    (!genmc::isa<CasReadLabel>(pLab) ||
				     static_cast<const ReadLabel *>(
					     static_cast<const EventLabel *>(pLab))
					     ->isRMW()) &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_38.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(38, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_38.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_39.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(39, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_39.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    pLab->isAtLeastAcquire() &&
				    (!genmc::isa<CasReadLabel>(pLab) ||
				     static_cast<const ReadLabel *>(
					     static_cast<const EventLabel *>(pLab))
					     ->isRMW()) &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_39.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(39, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_39.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    pLab->isAtLeastAcquire() &&
				    (!genmc::isa<CasReadLabel>(pLab) ||
				     static_cast<const ReadLabel *>(
					     static_cast<const EventLabel *>(pLab))
					     ->isRMW()) &&
				    genmc::isa<ReadLabel>(pLab) && !pLab->isLkmmNoreturn() &&
				    !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_41.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(41, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_41.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true &&
				    (genmc::isa<LockCasReadLabel>(pLab) ||
				     genmc::isa<TrylockCasReadLabel>(pLab)) &&
				    static_cast<const ReadLabel *>(
					    static_cast<const EventLabel *>(pLab))
					    ->isRMW()) {
					auto status = visitedConsAcyclic2_42.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(42, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_42.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true &&
				    (genmc::isa<LockCasReadLabel>(pLab) ||
				     genmc::isa<TrylockCasReadLabel>(pLab)) &&
				    static_cast<const ReadLabel *>(
					    static_cast<const EventLabel *>(pLab))
					    ->isRMW()) {
					auto status = visitedConsAcyclic2_43.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(43, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_43.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 40: {
			if (isFinishing) {
				visitedConsAcyclic2_40.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_40.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_40.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(40, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_40.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && genmc::isa<ReadLabel>(pLab) &&
				    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic()) &&
				    genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_20.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(20, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_20.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_20.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(20, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_20.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic()) &&
				    genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic()) &&
				    genmc::isa<ReadLabel>(pLab)) {
					auto status = visitedConsAcyclic2_30.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(30, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_30.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_33.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(33, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_33.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_34.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(34, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_34.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic2_35.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(35, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_35.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic()) &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_37.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(37, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_37.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    pLab->isAtLeastRelease() && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_38.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(38, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_38.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_39.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(39, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_39.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_40.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(40, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_40.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
					auto status = visitedConsAcyclic2_41.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(41, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_41.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 41: {
			if (isFinishing) {
				visitedConsAcyclic2_41.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_41.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_41.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(41, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_41.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
				    static_cast<const LkmmFenceLabel *>(
					    static_cast<const EventLabel *>(pLab))
						    ->getFenceKind() == LkmmFenceKind::Rmb) {
					auto status = visitedConsAcyclic2_40.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(40, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_40.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_41.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(41, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_41.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 42: {
			if (isFinishing) {
				visitedConsAcyclic2_42.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_42.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_42.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(42, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_42.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<UnlockWriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_38.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(38, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_38.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_42.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(42, pLab);
				} else if (status == NodeStatus::entered &&
					   (visitedConsAcyclic2Accepting >
						    visitedConsAcyclic2_42.getCount(
							    pLab->getStamp().get()) ||
					    0)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 43: {
			if (isFinishing) {
				visitedConsAcyclic2_43.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_43.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2_43.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::entered);
				worklist.emplace_back(43, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_43.getCount(
									   lab->getStamp().get()) ||
				    0)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rfi_pred(lab); pLab)
				if (true && genmc::isa<UnlockWriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_38.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(38, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_38.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 44: {
			if (isFinishing) {
				--visitedConsAcyclic2Accepting;
				visitedConsAcyclic2_44.set(lab->getStamp().get(),
							   visitedConsAcyclic2Accepting,
							   NodeStatus::left);
				break;
			}

			auto status = visitedConsAcyclic2_44.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				++visitedConsAcyclic2Accepting;
				visitedConsAcyclic2_44.setIncr(lab->getStamp().get(),
							       visitedConsAcyclic2Accepting,
							       NodeStatus::entered);
				worklist.emplace_back(44, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   (visitedConsAcyclic2Accepting > visitedConsAcyclic2_44.getCount(
									   lab->getStamp().get()) ||
				    1)) {
				return false; /* cycle detected */
			} else if (status == NodeStatus::left) {
				break; /* already explored*/
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_0.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(0, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_0.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_0.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(0, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_0.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_0.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(0, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_0.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_0.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(0, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_0.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_1.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(1, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_1.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_1.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(1, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_1.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_1.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(1, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_1.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_1.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(1, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_1.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() &&
					    genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
					    !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_10.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(10, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_10.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() &&
					    genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
					    !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_10.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(10, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_10.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_10.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(10, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_10.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_10.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(10, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_10.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_12.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(12, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_12.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_12.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(12, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_12.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_12.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(12, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_12.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_16.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(16, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_16.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_16.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(16, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_16.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_16.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(16, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_16.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_16.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(16, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_16.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_18.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(18, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_18.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_18.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(18, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_18.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_18.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(18, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_18.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab) &&
			    ((genmc::isa<ReadLabel>(lab) &&
			      genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
			     (genmc::isa<WriteLabel>(lab) &&
			      genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() ==
						    LkmmFenceKind::BeforeAtomic) {
						auto status = visitedConsAcyclic2_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && genmc::isa<ReadLabel>(pLab) &&
						    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_19.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(19, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_19.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() == LkmmFenceKind::Mb) {
						auto status = visitedConsAcyclic2_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab) &&
			    ((genmc::isa<ReadLabel>(lab) &&
			      genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
			     (genmc::isa<WriteLabel>(lab) &&
			      genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() ==
						    LkmmFenceKind::BeforeAtomic) {
						auto status = visitedConsAcyclic2_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && genmc::isa<ReadLabel>(pLab) &&
						    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_19.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(19, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_19.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.ctrl_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && genmc::isa<ReadLabel>(pLab) &&
						    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_19.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(19, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_19.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.data_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && genmc::isa<ReadLabel>(pLab) &&
						    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_19.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(19, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_19.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() == LkmmFenceKind::Mb) {
						auto status = visitedConsAcyclic2_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				for (auto &tmp : g.fri_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && genmc::isa<ReadLabel>(pLab) &&
						    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_19.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(19, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_19.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()))
				for (auto &tmp : g.moi_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && genmc::isa<ReadLabel>(pLab) &&
						    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_19.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(19, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_19.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()))
				for (auto *pLab : g.prop_int_preds(lab))
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isAtLeastAcquire() &&
					    (!genmc::isa<CasReadLabel>(pLab) ||
					     static_cast<const ReadLabel *>(
						     static_cast<const EventLabel *>(pLab))
						     ->isRMW()) &&
					    genmc::isa<ReadLabel>(pLab) && pLab->isLkmmMb() &&
					    !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.rfe_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.tc_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.tj_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic2_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_19.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab) && lab->isAtLeastRelease() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_19.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic()) &&
						    genmc::isa<ReadLabel>(pLab)) {
							auto status =
								visitedConsAcyclic2_20.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(20, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_20.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic2_20.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(20, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_20.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic()) &&
						    genmc::isa<ReadLabel>(pLab)) {
							auto status =
								visitedConsAcyclic2_20.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(20, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_20.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.ctrl_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic()) &&
						    genmc::isa<ReadLabel>(pLab)) {
							auto status =
								visitedConsAcyclic2_20.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(20, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_20.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.data_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic()) &&
						    genmc::isa<ReadLabel>(pLab)) {
							auto status =
								visitedConsAcyclic2_20.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(20, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_20.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic2_20.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(20, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_20.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				for (auto &tmp : g.fri_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic()) &&
						    genmc::isa<ReadLabel>(pLab)) {
							auto status =
								visitedConsAcyclic2_20.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(20, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_20.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()))
				for (auto &tmp : g.moi_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic()) &&
						    genmc::isa<WriteLabel>(pLab)) {
							auto status =
								visitedConsAcyclic2_20.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(20, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_20.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()))
				for (auto *pLab : g.prop_int_preds(lab))
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				for (auto *pLab : g.prop_int_preds(lab))
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isAtLeastAcquire() &&
					    (!genmc::isa<CasReadLabel>(pLab) ||
					     static_cast<const ReadLabel *>(
						     static_cast<const EventLabel *>(pLab))
						     ->isRMW()) &&
					    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isAtLeastAcquire() &&
					    (!genmc::isa<CasReadLabel>(pLab) ||
					     static_cast<const ReadLabel *>(
						     static_cast<const EventLabel *>(pLab))
						     ->isRMW()) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.rfe_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab) && lab->isAtLeastRelease() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab) && lab->isAtLeastRelease() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_20.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(20, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_20.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab) &&
			    ((genmc::isa<ReadLabel>(lab) &&
			      genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
			     (genmc::isa<WriteLabel>(lab) &&
			      genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic2_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic()) &&
						    genmc::isa<ReadLabel>(pLab) &&
						    ((genmc::isa<ReadLabel>(pLab) &&
						      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
						     (genmc::isa<WriteLabel>(pLab) &&
						      genmc::dyn_cast<WriteLabel>(pLab)
							      ->isRMW()))) {
							auto status =
								visitedConsAcyclic2_21.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(21, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_21.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab) &&
			    ((genmc::isa<ReadLabel>(lab) &&
			      genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
			     (genmc::isa<WriteLabel>(lab) &&
			      genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic2_21.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(21, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_21.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic()) &&
						    genmc::isa<ReadLabel>(pLab) &&
						    ((genmc::isa<ReadLabel>(pLab) &&
						      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
						     (genmc::isa<WriteLabel>(pLab) &&
						      genmc::dyn_cast<WriteLabel>(pLab)
							      ->isRMW()))) {
							auto status =
								visitedConsAcyclic2_21.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(21, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_21.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.ctrl_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic()) &&
						    genmc::isa<ReadLabel>(pLab) &&
						    ((genmc::isa<ReadLabel>(pLab) &&
						      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
						     (genmc::isa<WriteLabel>(pLab) &&
						      genmc::dyn_cast<WriteLabel>(pLab)
							      ->isRMW()))) {
							auto status =
								visitedConsAcyclic2_21.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(21, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_21.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.data_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic()) &&
						    genmc::isa<ReadLabel>(pLab) &&
						    ((genmc::isa<ReadLabel>(pLab) &&
						      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
						     (genmc::isa<WriteLabel>(pLab) &&
						      genmc::dyn_cast<WriteLabel>(pLab)
							      ->isRMW()))) {
							auto status =
								visitedConsAcyclic2_21.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(21, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_21.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				for (auto &tmp : g.fri_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic()) &&
						    genmc::isa<ReadLabel>(pLab) &&
						    ((genmc::isa<ReadLabel>(pLab) &&
						      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
						     (genmc::isa<WriteLabel>(pLab) &&
						      genmc::dyn_cast<WriteLabel>(pLab)
							      ->isRMW()))) {
							auto status =
								visitedConsAcyclic2_21.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(21, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_21.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()))
				for (auto &tmp : g.moi_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic()) &&
						    genmc::isa<WriteLabel>(pLab) &&
						    ((genmc::isa<ReadLabel>(pLab) &&
						      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
						     (genmc::isa<WriteLabel>(pLab) &&
						      genmc::dyn_cast<WriteLabel>(pLab)
							      ->isRMW()))) {
							auto status =
								visitedConsAcyclic2_21.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(21, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_21.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()))
				for (auto *pLab : g.prop_int_preds(lab))
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				for (auto *pLab : g.prop_int_preds(lab))
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isAtLeastAcquire() &&
					    (!genmc::isa<CasReadLabel>(pLab) ||
					     static_cast<const ReadLabel *>(
						     static_cast<const EventLabel *>(pLab))
						     ->isRMW()) &&
					    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isAtLeastAcquire() &&
					    (!genmc::isa<CasReadLabel>(pLab) ||
					     static_cast<const ReadLabel *>(
						     static_cast<const EventLabel *>(pLab))
						     ->isRMW()) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.rfe_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab) && lab->isAtLeastRelease() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab) && lab->isAtLeastRelease() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_21.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(21, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_21.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() ==
						    LkmmFenceKind::AfterAtomic) {
						auto status = visitedConsAcyclic2_22.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(22, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_22.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() ==
						    LkmmFenceKind::AfterAtomic) {
						auto status = visitedConsAcyclic2_22.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(22, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_22.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() ==
						    LkmmFenceKind::AfterSpinlock) {
						auto status = visitedConsAcyclic2_23.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(23, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_23.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() ==
						    LkmmFenceKind::AfterSpinlock) {
						auto status = visitedConsAcyclic2_23.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(23, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_23.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() ==
						    LkmmFenceKind::AfterUnlockLock) {
						auto status = visitedConsAcyclic2_26.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(26, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_26.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() ==
						    LkmmFenceKind::AfterUnlockLock) {
						auto status = visitedConsAcyclic2_26.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(26, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_26.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true) {
						auto status = visitedConsAcyclic2_29.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(29, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_29.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true) {
						auto status = visitedConsAcyclic2_29.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(29, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_29.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.ctrl_preds(lab))
					if (auto *pLab = &tmp; true) {
						auto status = visitedConsAcyclic2_29.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(29, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_29.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.data_preds(lab))
					if (auto *pLab = &tmp; true) {
						auto status = visitedConsAcyclic2_29.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(29, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_29.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic()) &&
						    genmc::isa<ReadLabel>(pLab)) {
							auto status =
								visitedConsAcyclic2_30.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(30, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_30.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_30.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(30, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_30.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic()) &&
						    genmc::isa<ReadLabel>(pLab)) {
							auto status =
								visitedConsAcyclic2_30.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(30, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_30.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.ctrl_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic()) &&
						    genmc::isa<ReadLabel>(pLab)) {
							auto status =
								visitedConsAcyclic2_30.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(30, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_30.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.data_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic()) &&
						    genmc::isa<ReadLabel>(pLab)) {
							auto status =
								visitedConsAcyclic2_30.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(30, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_30.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_30.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(30, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_30.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				for (auto &tmp : g.fri_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic()) &&
						    genmc::isa<ReadLabel>(pLab)) {
							auto status =
								visitedConsAcyclic2_30.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(30, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_30.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()))
				for (auto *pLab : g.prop_int_preds(lab))
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_30.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(30, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_30.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isAtLeastAcquire() &&
					    (!genmc::isa<CasReadLabel>(pLab) ||
					     static_cast<const ReadLabel *>(
						     static_cast<const EventLabel *>(pLab))
						     ->isRMW()) &&
					    !(pLab->isNotAtomic()) && genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_30.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(30, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_30.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_30.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(30, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_30.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab) && lab->isAtLeastRelease() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_30.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(30, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_30.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.rfi_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_31.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(31, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_31.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.rfi_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_32.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(32, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_32.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_33.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(33, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_33.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_33.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(33, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_33.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_33.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(33, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_33.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.ctrl_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_33.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(33, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_33.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.data_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_33.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(33, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_33.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_33.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(33, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_33.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				for (auto &tmp : g.fri_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_33.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(33, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_33.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()))
				for (auto &tmp : g.moi_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_33.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(33, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_33.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()))
				for (auto *pLab : g.prop_int_preds(lab))
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_33.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(33, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_33.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isAtLeastAcquire() &&
					    (!genmc::isa<CasReadLabel>(pLab) ||
					     static_cast<const ReadLabel *>(
						     static_cast<const EventLabel *>(pLab))
						     ->isRMW()) &&
					    !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_33.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(33, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_33.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.rfe_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_33.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(33, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_33.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.tc_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_33.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(33, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_33.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.tj_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_33.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(33, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_33.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_33.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(33, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_33.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_33.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(33, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_33.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab) && lab->isAtLeastRelease() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_33.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(33, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_33.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_34.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(34, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_34.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab) &&
			    ((genmc::isa<ReadLabel>(lab) &&
			      genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
			     (genmc::isa<WriteLabel>(lab) &&
			      genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_34.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(34, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_34.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_34.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(34, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_34.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				for (auto &tmp : g.moi_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic()) &&
						    genmc::isa<WriteLabel>(pLab)) {
							auto status =
								visitedConsAcyclic2_34.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(34, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_34.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()))
				for (auto *pLab : g.prop_int_preds(lab))
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_34.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(34, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_34.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isAtLeastAcquire() &&
					    (!genmc::isa<CasReadLabel>(pLab) ||
					     static_cast<const ReadLabel *>(
						     static_cast<const EventLabel *>(pLab))
						     ->isRMW()) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_34.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(34, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_34.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.rfe_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_34.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(34, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_34.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_34.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(34, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_34.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab) && lab->isAtLeastRelease() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_34.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(34, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_34.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_35.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(35, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_35.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_35.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(35, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_35.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				for (auto &tmp : g.moi_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic()) &&
						    genmc::isa<WriteLabel>(pLab) &&
						    ((genmc::isa<ReadLabel>(pLab) &&
						      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
						     (genmc::isa<WriteLabel>(pLab) &&
						      genmc::dyn_cast<WriteLabel>(pLab)
							      ->isRMW()))) {
							auto status =
								visitedConsAcyclic2_35.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(35, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_35.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()))
				for (auto *pLab : g.prop_int_preds(lab))
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_35.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(35, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_35.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isAtLeastAcquire() &&
					    (!genmc::isa<CasReadLabel>(pLab) ||
					     static_cast<const ReadLabel *>(
						     static_cast<const EventLabel *>(pLab))
						     ->isRMW()) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_35.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(35, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_35.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.rfe_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_35.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(35, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_35.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_35.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(35, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_35.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab) && lab->isAtLeastRelease() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						auto status = visitedConsAcyclic2_35.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(35, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_35.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() ==
						    LkmmFenceKind::Wmb) {
						auto status = visitedConsAcyclic2_36.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(36, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_36.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_37.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(37, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_37.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic2_37.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(37, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_37.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_37.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(37, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_37.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				for (auto &tmp : g.moi_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic()) &&
						    genmc::isa<WriteLabel>(pLab)) {
							auto status =
								visitedConsAcyclic2_37.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(37, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_37.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()))
				for (auto *pLab : g.prop_int_preds(lab))
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_37.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(37, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_37.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isAtLeastAcquire() &&
					    (!genmc::isa<CasReadLabel>(pLab) ||
					     static_cast<const ReadLabel *>(
						     static_cast<const EventLabel *>(pLab))
						     ->isRMW()) &&
					    !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_37.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(37, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_37.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.rfe_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_37.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(37, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_37.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_37.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(37, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_37.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab) && lab->isAtLeastRelease() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_37.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(37, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_37.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && genmc::isa<WriteLabel>(pLab) &&
						    pLab->isAtLeastRelease() &&
						    !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_38.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(38, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_38.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() &&
					    genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
					    !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_38.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(38, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_38.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && genmc::isa<WriteLabel>(pLab) &&
						    pLab->isAtLeastRelease() &&
						    !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_38.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(38, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_38.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.ctrl_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && genmc::isa<WriteLabel>(pLab) &&
						    pLab->isAtLeastRelease() &&
						    !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_38.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(38, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_38.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.data_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && genmc::isa<WriteLabel>(pLab) &&
						    pLab->isAtLeastRelease() &&
						    !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_38.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(38, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_38.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() &&
					    genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
					    !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_38.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(38, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_38.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				for (auto &tmp : g.fri_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && genmc::isa<WriteLabel>(pLab) &&
						    pLab->isAtLeastRelease() &&
						    !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_38.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(38, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_38.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()))
				for (auto &tmp : g.moi_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && genmc::isa<WriteLabel>(pLab) &&
						    pLab->isAtLeastRelease() &&
						    !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_38.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(38, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_38.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()))
				for (auto *pLab : g.prop_int_preds(lab))
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_38.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(38, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_38.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() &&
					    genmc::isa<ReadLabel>(pLab) &&
					    pLab->isAtLeastAcquire() &&
					    (!genmc::isa<CasReadLabel>(pLab) ||
					     static_cast<const ReadLabel *>(
						     static_cast<const EventLabel *>(pLab))
						     ->isRMW()) &&
					    !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_38.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(38, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_38.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.rfe_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_38.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(38, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_38.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.tc_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_38.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(38, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_38.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.tj_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_38.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(38, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_38.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_38.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(38, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_38.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_38.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(38, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_38.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab) && lab->isAtLeastRelease() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic2_38.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(38, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_38.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (true && genmc::isa<WriteLabel>(lab) && lab->isAtLeastRelease() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isAtLeastRelease() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_38.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(38, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_38.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_39.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(39, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_39.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_39.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(39, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_39.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_39.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(39, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_39.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.ctrl_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_39.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(39, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_39.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.data_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_39.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(39, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_39.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_39.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(39, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_39.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				for (auto &tmp : g.fri_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_39.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(39, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_39.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()))
				for (auto &tmp : g.moi_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_39.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(39, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_39.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()))
				for (auto *pLab : g.prop_int_preds(lab))
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_39.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(39, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_39.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic2_39.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(39, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_39.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isAtLeastAcquire() &&
					    (!genmc::isa<CasReadLabel>(pLab) ||
					     static_cast<const ReadLabel *>(
						     static_cast<const EventLabel *>(pLab))
						     ->isRMW()) &&
					    !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_39.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(39, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_39.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.rfe_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_39.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(39, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_39.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.tc_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_39.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(39, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_39.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.tj_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_39.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(39, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_39.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_39.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(39, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_39.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_39.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(39, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_39.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab) && lab->isAtLeastRelease() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_39.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(39, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_39.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && !lab->isLkmmNoreturn() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
					    static_cast<const LkmmFenceLabel *>(
						    static_cast<const EventLabel *>(pLab))
							    ->getFenceKind() ==
						    LkmmFenceKind::Rmb) {
						auto status = visitedConsAcyclic2_40.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(40, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_40.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && genmc::isa<ReadLabel>(pLab) &&
						    !pLab->isLkmmNoreturn() &&
						    !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_41.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(41, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_41.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    !pLab->isLkmmNoreturn() &&
					    genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
					    !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_41.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(41, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_41.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && genmc::isa<ReadLabel>(pLab) &&
						    !pLab->isLkmmNoreturn() &&
						    !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_41.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(41, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_41.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.ctrl_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && genmc::isa<ReadLabel>(pLab) &&
						    !pLab->isLkmmNoreturn() &&
						    !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_41.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(41, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_41.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.data_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && genmc::isa<ReadLabel>(pLab) &&
						    !pLab->isLkmmNoreturn() &&
						    !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_41.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(41, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_41.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    !pLab->isLkmmNoreturn() &&
					    genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb() &&
					    !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_41.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(41, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_41.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				for (auto &tmp : g.fri_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && genmc::isa<ReadLabel>(pLab) &&
						    !pLab->isLkmmNoreturn() &&
						    !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_41.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(41, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_41.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()))
				for (auto &tmp : g.moi_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && genmc::isa<ReadLabel>(pLab) &&
						    !pLab->isLkmmNoreturn() &&
						    !(pLab->isNotAtomic())) {
							auto status =
								visitedConsAcyclic2_41.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(41, pLab);
							} else if (
								status == NodeStatus::entered &&
								(visitedConsAcyclic2Accepting >
									 visitedConsAcyclic2_41.getCount(
										 pLab->getStamp()
											 .get()) ||
								 0)) {
								return false; /* cycle detected */
							}
						}
			if (true && !(lab->isNotAtomic()))
				for (auto *pLab : g.prop_int_preds(lab))
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_41.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(41, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_41.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    pLab->isAtLeastAcquire() &&
					    (!genmc::isa<CasReadLabel>(pLab) ||
					     static_cast<const ReadLabel *>(
						     static_cast<const EventLabel *>(pLab))
						     ->isRMW()) &&
					    genmc::isa<ReadLabel>(pLab) &&
					    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_41.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(41, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_41.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.rfe_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_41.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(41, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_41.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.tc_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_41.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(41, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_41.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.tj_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_41.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(41, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_41.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && !lab->isLkmmNoreturn() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic2_41.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(41, pLab);
					} else if (status == NodeStatus::entered &&
						   (visitedConsAcyclic2Accepting >
							    visitedConsAcyclic2_41.getCount(
								    pLab->getStamp().get()) ||
						    0)) {
						return false; /* cycle detected */
					}
				}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_41.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(41, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_41.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_41.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(41, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_41.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab) && lab->isAtLeastRelease() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    !pLab->isLkmmNoreturn() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_41.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(41, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_41.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true &&
					    (genmc::isa<LockCasReadLabel>(pLab) ||
					     genmc::isa<TrylockCasReadLabel>(pLab)) &&
					    static_cast<const ReadLabel *>(
						    static_cast<const EventLabel *>(pLab))
						    ->isRMW()) {
						auto status = visitedConsAcyclic2_42.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(42, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_42.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true &&
					    (genmc::isa<LockCasReadLabel>(pLab) ||
					     genmc::isa<TrylockCasReadLabel>(pLab)) &&
					    static_cast<const ReadLabel *>(
						    static_cast<const EventLabel *>(pLab))
						    ->isRMW()) {
						auto status = visitedConsAcyclic2_43.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(43, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_43.getCount(
									    pLab->getStamp()
										    .get()) ||
							    0)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<ReadLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_44.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(44, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_44.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    pLab->isLkmmMb() && !(pLab->isNotAtomic())) {
						auto status = visitedConsAcyclic2_44.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(44, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_44.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<ReadLabel>(pLab)) {
						auto status = visitedConsAcyclic2_44.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(44, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_44.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb() &&
			    !(lab->isNotAtomic()))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && !(pLab->isNotAtomic()) &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_44.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(44, pLab);
						} else if (status == NodeStatus::entered &&
							   (visitedConsAcyclic2Accepting >
								    visitedConsAcyclic2_44.getCount(
									    pLab->getStamp()
										    .get()) ||
							    1)) {
							return false; /* cycle detected */
						}
					}

			break;
		}
		default:
			UNREACHABLE();
		}
	}
	return true;
}

bool LKMMChecker::visitConsAcyclic2(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedConsAcyclic2Accepting = 0;
	visitedConsAcyclic2_0.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_1.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_2.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_3.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_4.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_5.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_6.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_7.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_9.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_10.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_11.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_12.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_13.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_14.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_16.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_18.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_19.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_20.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_21.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_22.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_23.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_24.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_26.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_27.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_29.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_30.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_31.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_32.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_33.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_34.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_35.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_36.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_37.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_38.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_39.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_40.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_41.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_42.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_43.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_44.maybeClearResize(g.getMaxStamp().get() + 1);

	/* States we need to explore from using DFS */
	std::vector<DFSWorklistEntry> startStates = {
		{44, lab}, {41, lab}, {39, lab}, {38, lab}, {33, lab}, {19, lab},
	};

	return visitConsAcyclic2Iterative(startStates);
}

bool LKMMChecker::visitConsAcyclic2Full(const ExecutionGraph &g) const
{
	visitedConsAcyclic2Accepting = 0;
	visitedConsAcyclic2_0.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_1.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_2.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_3.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_4.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_5.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_6.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_7.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_9.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_10.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_11.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_12.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_13.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_14.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_16.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_18.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_19.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_20.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_21.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_22.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_23.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_24.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_26.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_27.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_29.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_30.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_31.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_32.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_33.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_34.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_35.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_36.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_37.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_38.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_39.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_40.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_41.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_42.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_43.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_44.maybeClearResize(g.getMaxStamp().get() + 1);

	auto exploreLab = [&](auto &lab) {
		/* Explore from all accepting states using DFS */
		std::vector<DFSWorklistEntry> startStates = {
			{44, &lab},
		};

		return visitConsAcyclic2Iterative(startStates);
	};

	return std::ranges::all_of(g.labels(), exploreLab);
}

bool LKMMChecker::checkConsAcyclic2(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	return visitConsAcyclic2(lab);
}

bool LKMMChecker::checkConsAcyclic2(const ExecutionGraph &g) const
{
	return visitConsAcyclic2Full(g);
}

bool LKMMChecker::visitError3([[maybe_unused]] const EventLabel *lab) const { return false; }

bool LKMMChecker::visitLHSUnlessError3Iterative(std::vector<DFSWorklistEntry> &worklist,
						const View &v) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		case 0: {
			if (isFinishing) {
				break;
			}

			if (!v.contains(lab->getPos())) {
				cexLab = lab;
				return false;
			}

			[[maybe_unused]] auto &g = *lab->getParent();

			break;
		}
		case 1: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto tmpE = g.alloc_pos(lab); tmpE) {
				if (!v.contains(*tmpE)) {
					cexLab = g.getEventLabelIfPresent(*tmpE);
					return false;
				}
			}

			break;
		}
		default:
			UNREACHABLE();
		}
	}
	return true;
}

bool LKMMChecker::visitUnlessError3(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedLHSUnlessError3Accepting.clear();
	visitedLHSUnlessError3Accepting.resize(g.getMaxStamp().get() + 1, false);

	auto &v = lab->view(1);

	/* Explore from all accepting states in LHS using DFS */
	std::vector<DFSWorklistEntry> startStatesLHS = {
		{1, lab},
	};

	return visitLHSUnlessError3Iterative(startStatesLHS, v);
}

bool LKMMChecker::checkError3(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	if (visitUnlessError3(lab))
		return true;

	return visitError3(lab);
}

bool LKMMChecker::visitError4([[maybe_unused]] const EventLabel *lab) const { return false; }

bool LKMMChecker::visitLHSUnlessError4Iterative(std::vector<DFSWorklistEntry> &worklist) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		case 0: {
			if (isFinishing) {
				break;
			}

			return false;
			[[maybe_unused]] auto &g = *lab->getParent();

			break;
		}
		case 1: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (true && genmc::isa<FreeLabel>(lab) && !genmc::isa<HpRetireLabel>(lab))
				if (auto tmpE = g.free_pos(lab); tmpE) {
					return false;
				}
			if (true && genmc::isa<HpRetireLabel>(lab))
				if (auto tmpE = g.free_pos(lab); tmpE) {
					return false;
				}

			break;
		}
		default:
			UNREACHABLE();
		}
	}
	return true;
}

bool LKMMChecker::visitRHSUnlessError4Iterative(std::vector<DFSWorklistEntry> &worklist) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		default:
			UNREACHABLE();
		}
	}
	return true;
}

bool LKMMChecker::visitUnlessError4(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedLHSUnlessError4Accepting.clear();
	visitedLHSUnlessError4Accepting.resize(g.getMaxStamp().get() + 1, false);
	visitedRHSUnlessError4Accepting.clear();
	visitedRHSUnlessError4Accepting.resize(g.getMaxStamp().get() + 1, false);

	/* Explore from all accepting states in LHS using DFS */
	std::vector<DFSWorklistEntry> startStatesLHS = {
		{1, lab},
	};

	if (!visitLHSUnlessError4Iterative(startStatesLHS))
		return false;

	for (auto i = 0u; i < visitedLHSUnlessError4Accepting.size(); i++) {
		if (visitedLHSUnlessError4Accepting[i] && !visitedRHSUnlessError4Accepting[i]) {
			cexLab = &*std::find_if(g.label_begin(), g.label_end(),
						[&](auto &lab) { return lab.getStamp() == i; });
			return false;
		}
	}
	return true;
}

bool LKMMChecker::checkError4(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	if (visitUnlessError4(lab))
		return true;

	return visitError4(lab);
}

bool LKMMChecker::visitError5([[maybe_unused]] const EventLabel *lab) const { return false; }

bool LKMMChecker::visitLHSUnlessError5Iterative(std::vector<DFSWorklistEntry> &worklist,
						const View &v) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		case 0: {
			if (isFinishing) {
				break;
			}

			if (!v.contains(lab->getPos())) {
				cexLab = lab;
				return false;
			}

			[[maybe_unused]] auto &g = *lab->getParent();

			break;
		}
		case 1: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (true && genmc::isa<FreeLabel>(lab) && !genmc::isa<HpRetireLabel>(lab))
				for (auto tmpE : g.pomax_at_reads_pos(lab)) {
					if (!v.contains(tmpE)) {
						cexLab = g.getEventLabelIfPresent(tmpE);
						return false;
					}
				}
			if (true && genmc::isa<FreeLabel>(lab) && !genmc::isa<HpRetireLabel>(lab))
				for (auto tmpE : g.pomax_at_writes_pos(lab)) {
					if (!v.contains(tmpE)) {
						cexLab = g.getEventLabelIfPresent(tmpE);
						return false;
					}
				}
			if (true && genmc::isa<FreeLabel>(lab) && !genmc::isa<HpRetireLabel>(lab))
				for (auto tmpE : g.pomax_na_reads_pos(lab)) {
					if (!v.contains(tmpE)) {
						cexLab = g.getEventLabelIfPresent(tmpE);
						return false;
					}
				}
			if (true && genmc::isa<FreeLabel>(lab) && !genmc::isa<HpRetireLabel>(lab))
				for (auto tmpE : g.pomax_na_writes_pos(lab)) {
					if (!v.contains(tmpE)) {
						cexLab = g.getEventLabelIfPresent(tmpE);
						return false;
					}
				}

			break;
		}
		default:
			UNREACHABLE();
		}
	}
	return true;
}

bool LKMMChecker::visitUnlessError5(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedLHSUnlessError5Accepting.clear();
	visitedLHSUnlessError5Accepting.resize(g.getMaxStamp().get() + 1, false);

	auto &v = lab->view(1);

	/* Explore from all accepting states in LHS using DFS */
	std::vector<DFSWorklistEntry> startStatesLHS = {
		{1, lab},
	};

	return visitLHSUnlessError5Iterative(startStatesLHS, v);
}

bool LKMMChecker::checkError5(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	if (visitUnlessError5(lab))
		return true;

	return visitError5(lab);
}

bool LKMMChecker::visitError6([[maybe_unused]] const EventLabel *lab) const { return false; }

bool LKMMChecker::visitLHSUnlessError6Iterative(std::vector<DFSWorklistEntry> &worklist) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		case 0: {
			if (isFinishing) {
				break;
			}

			return false;
			[[maybe_unused]] auto &g = *lab->getParent();

			break;
		}
		case 1: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (true && genmc::isa<ReadLabel>(lab))
				if (auto tmpE = g.free_pos(lab); tmpE) {
					return false;
				}
			if (true && genmc::isa<WriteLabel>(lab))
				if (auto tmpE = g.free_pos(lab); tmpE) {
					return false;
				}

			break;
		}
		default:
			UNREACHABLE();
		}
	}
	return true;
}

bool LKMMChecker::visitRHSUnlessError6Iterative(std::vector<DFSWorklistEntry> &worklist) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		default:
			UNREACHABLE();
		}
	}
	return true;
}

bool LKMMChecker::visitUnlessError6(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedLHSUnlessError6Accepting.clear();
	visitedLHSUnlessError6Accepting.resize(g.getMaxStamp().get() + 1, false);
	visitedRHSUnlessError6Accepting.clear();
	visitedRHSUnlessError6Accepting.resize(g.getMaxStamp().get() + 1, false);

	/* Explore from all accepting states in LHS using DFS */
	std::vector<DFSWorklistEntry> startStatesLHS = {
		{1, lab},
	};

	if (!visitLHSUnlessError6Iterative(startStatesLHS))
		return false;

	for (auto i = 0u; i < visitedLHSUnlessError6Accepting.size(); i++) {
		if (visitedLHSUnlessError6Accepting[i] && !visitedRHSUnlessError6Accepting[i]) {
			cexLab = &*std::find_if(g.label_begin(), g.label_end(),
						[&](auto &lab) { return lab.getStamp() == i; });
			return false;
		}
	}
	return true;
}

bool LKMMChecker::checkError6(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	if (visitUnlessError6(lab))
		return true;

	return visitError6(lab);
}

bool LKMMChecker::visitError7([[maybe_unused]] const EventLabel *lab) const { return false; }

bool LKMMChecker::visitLHSUnlessError7Iterative(std::vector<DFSWorklistEntry> &worklist,
						const View &v) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		case 0: {
			if (isFinishing) {
				break;
			}

			if (!v.contains(lab->getPos())) {
				cexLab = lab;
				return false;
			}

			[[maybe_unused]] auto &g = *lab->getParent();

			break;
		}
		case 1: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (true && genmc::isa<HpRetireLabel>(lab))
				for (auto tmpE : g.unprotected_pos(lab)) {
					if (!v.contains(tmpE)) {
						cexLab = g.getEventLabelIfPresent(tmpE);
						return false;
					}
				}

			break;
		}
		default:
			UNREACHABLE();
		}
	}
	return true;
}

bool LKMMChecker::visitUnlessError7(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedLHSUnlessError7Accepting.clear();
	visitedLHSUnlessError7Accepting.resize(g.getMaxStamp().get() + 1, false);

	auto &v = lab->view(1);

	/* Explore from all accepting states in LHS using DFS */
	std::vector<DFSWorklistEntry> startStatesLHS = {
		{1, lab},
	};

	return visitLHSUnlessError7Iterative(startStatesLHS, v);
}

bool LKMMChecker::checkError7(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	if (visitUnlessError7(lab))
		return true;

	return visitError7(lab);
}

bool LKMMChecker::visitError8([[maybe_unused]] const EventLabel *lab) const { return false; }

bool LKMMChecker::visitLHSUnlessError8Iterative(std::vector<DFSWorklistEntry> &worklist) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		case 0: {
			if (isFinishing) {
				break;
			}

			return false;
			[[maybe_unused]] auto &g = *lab->getParent();

			break;
		}
		case 1: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (true && genmc::isa<MemAccessLabel>(lab) &&
			    genmc::dyn_cast<MemAccessLabel>(lab)->getAddr().isDynamic() &&
			    g.retire_pos(lab).has_value() &&
			    !isHazptrProtected(genmc::dyn_cast<MemAccessLabel>(lab)))
				if (auto tmpE = g.retire_pos(lab); tmpE) {
					return false;
				}

			break;
		}
		default:
			UNREACHABLE();
		}
	}
	return true;
}

bool LKMMChecker::visitRHSUnlessError8Iterative(std::vector<DFSWorklistEntry> &worklist) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		default:
			UNREACHABLE();
		}
	}
	return true;
}

bool LKMMChecker::visitUnlessError8(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedLHSUnlessError8Accepting.clear();
	visitedLHSUnlessError8Accepting.resize(g.getMaxStamp().get() + 1, false);
	visitedRHSUnlessError8Accepting.clear();
	visitedRHSUnlessError8Accepting.resize(g.getMaxStamp().get() + 1, false);

	/* Explore from all accepting states in LHS using DFS */
	std::vector<DFSWorklistEntry> startStatesLHS = {
		{1, lab},
	};

	if (!visitLHSUnlessError8Iterative(startStatesLHS))
		return false;

	for (auto i = 0u; i < visitedLHSUnlessError8Accepting.size(); i++) {
		if (visitedLHSUnlessError8Accepting[i] && !visitedRHSUnlessError8Accepting[i]) {
			cexLab = &*std::find_if(g.label_begin(), g.label_end(),
						[&](auto &lab) { return lab.getStamp() == i; });
			return false;
		}
	}
	return true;
}

bool LKMMChecker::checkError8(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	if (visitUnlessError8(lab))
		return true;

	return visitError8(lab);
}

bool LKMMChecker::visitError9([[maybe_unused]] const EventLabel *lab) const { return false; }

bool LKMMChecker::visitLHSUnlessError9Iterative(std::vector<DFSWorklistEntry> &worklist,
						const View &v) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		case 0: {
			if (isFinishing) {
				break;
			}

			if (!v.contains(lab->getPos())) {
				cexLab = lab;
				return false;
			}

			[[maybe_unused]] auto &g = *lab->getParent();

			break;
		}
		case 1: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (true && genmc::isa<ReadLabel>(lab))
				for (auto tmpE : g.pomax_na_writes_pos(lab)) {
					if (!v.contains(tmpE)) {
						cexLab = g.getEventLabelIfPresent(tmpE);
						return false;
					}
				}
			if (true && genmc::isa<WriteLabel>(lab))
				for (auto tmpE : g.pomax_na_reads_pos(lab)) {
					if (!v.contains(tmpE)) {
						cexLab = g.getEventLabelIfPresent(tmpE);
						return false;
					}
				}
			if (true && genmc::isa<WriteLabel>(lab))
				for (auto tmpE : g.pomax_na_writes_pos(lab)) {
					if (!v.contains(tmpE)) {
						cexLab = g.getEventLabelIfPresent(tmpE);
						return false;
					}
				}
			if (true && lab->isNotAtomic() && genmc::isa<ReadLabel>(lab))
				for (auto tmpE : g.pomax_at_writes_pos(lab)) {
					if (!v.contains(tmpE)) {
						cexLab = g.getEventLabelIfPresent(tmpE);
						return false;
					}
				}
			if (true && lab->isNotAtomic() && genmc::isa<WriteLabel>(lab))
				for (auto tmpE : g.pomax_at_reads_pos(lab)) {
					if (!v.contains(tmpE)) {
						cexLab = g.getEventLabelIfPresent(tmpE);
						return false;
					}
				}
			if (true && lab->isNotAtomic() && genmc::isa<WriteLabel>(lab))
				for (auto tmpE : g.pomax_at_writes_pos(lab)) {
					if (!v.contains(tmpE)) {
						cexLab = g.getEventLabelIfPresent(tmpE);
						return false;
					}
				}

			break;
		}
		default:
			UNREACHABLE();
		}
	}
	return true;
}

bool LKMMChecker::visitUnlessError9(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedLHSUnlessError9Accepting.clear();
	visitedLHSUnlessError9Accepting.resize(g.getMaxStamp().get() + 1, false);

	auto &v = lab->view(1);

	/* Explore from all accepting states in LHS using DFS */
	std::vector<DFSWorklistEntry> startStatesLHS = {
		{1, lab},
	};

	return visitLHSUnlessError9Iterative(startStatesLHS, v);
}

bool LKMMChecker::checkError9(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	if (visitUnlessError9(lab))
		return true;

	return visitError9(lab);
}

bool LKMMChecker::visitWarning10([[maybe_unused]] const EventLabel *lab) const { return false; }

bool LKMMChecker::visitLHSUnlessWarning10Iterative(std::vector<DFSWorklistEntry> &worklist,
						   const View &v) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		case 0: {
			if (isFinishing) {
				break;
			}

			if (!v.contains(lab->getPos())) {
				cexLab = lab;
				return false;
			}

			[[maybe_unused]] auto &g = *lab->getParent();

			break;
		}
		case 1: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (true && !(lab->isNotAtomic()) && genmc::isa<WriteLabel>(lab))
				for (auto tmpE : g.pomax_at_writes_pos(lab)) {
					if (!v.contains(tmpE)) {
						cexLab = g.getEventLabelIfPresent(tmpE);
						return false;
					}
				}

			break;
		}
		default:
			UNREACHABLE();
		}
	}
	return true;
}

bool LKMMChecker::visitUnlessWarning10(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedLHSUnlessWarning10Accepting.clear();
	visitedLHSUnlessWarning10Accepting.resize(g.getMaxStamp().get() + 1, false);

	auto &v = lab->view(1);

	/* Explore from all accepting states in LHS using DFS */
	std::vector<DFSWorklistEntry> startStatesLHS = {
		{1, lab},
	};

	return visitLHSUnlessWarning10Iterative(startStatesLHS, v);
}

bool LKMMChecker::checkWarning10(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	if (visitUnlessWarning10(lab))
		return true;

	return visitWarning10(lab);
}

std::optional<VerificationError>
LKMMChecker::checkErrors([[maybe_unused]] const EventLabel *lab,
			 [[maybe_unused]] const EventLabel *&race) const
{
	if (!checkError3(lab)) {
		race = cexLab;
		return {VerificationError::VE_AccessNonMalloc};
	}

	if (!checkError4(lab)) {
		race = cexLab;
		return {VerificationError::VE_DoubleFree};
	}

	if (!checkError5(lab)) {
		race = cexLab;
		return {VerificationError::VE_AccessFreed};
	}

	if (!checkError6(lab)) {
		race = cexLab;
		return {VerificationError::VE_AccessFreed};
	}

	if (!checkError7(lab)) {
		race = cexLab;
		return {VerificationError::VE_AccessFreed};
	}

	if (!checkError8(lab)) {
		race = cexLab;
		return {VerificationError::VE_AccessFreed};
	}

	if (!checkError9(lab)) {
		race = cexLab;
		return {VerificationError::VE_RaceNotAtomic};
	}

	return {};
}

std::vector<VerificationError>
LKMMChecker::checkWarnings(const EventLabel *lab, const VSet<VerificationError> &seenWarnings,
			   std::vector<const EventLabel *> &racyLabs) const
{
	std::vector<VerificationError> result;

	if (seenWarnings.count(VerificationError::VE_WWRace) == 0 && !checkWarning10(lab)) {
		racyLabs.push_back(cexLab);
		result.push_back(VerificationError::VE_WWRace);
	}

	return result;
}

bool LKMMChecker::isConsistent([[maybe_unused]] const EventLabel *lab) const
{

	return true && checkConsAcyclic1(lab) && checkConsAcyclic2(lab);
}

bool LKMMChecker::isConsistent([[maybe_unused]] const ExecutionGraph &g) const
{

	return true && checkConsAcyclic1(g) && checkConsAcyclic2(g);
}

bool LKMMChecker::isCoherentRelinche(const ExecutionGraph &g) const
{

	return true && visitCoherenceRelinche(g);
}

void LKMMChecker::visitPPoRf0(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf0[lab->getStamp().get()] = NodeStatus::entered;
	pporf.updateIdx(lab->getPos());
	for (auto &tmp : g.ctrl_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	for (auto &tmp : g.addr_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	for (auto &tmp : g.data_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	for (auto &tmp : g.ctrl_preds(lab))
		if (auto *pLab = &tmp; true)
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf7[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf7(pLab, pporf);
			}
	for (auto &tmp : g.addr_preds(lab))
		if (auto *pLab = &tmp; true)
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf7[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf7(pLab, pporf);
			}
	for (auto &tmp : g.data_preds(lab))
		if (auto *pLab = &tmp; true)
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf7[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf7(pLab, pporf);
			}
	for (auto &tmp : g.ctrl_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf10[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf10(pLab, pporf);
		}
	for (auto &tmp : g.addr_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf10[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf10(pLab, pporf);
		}
	for (auto &tmp : g.data_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf10[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf10(pLab, pporf);
		}
	for (auto &tmp : g.ctrl_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	for (auto &tmp : g.addr_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	for (auto &tmp : g.data_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	visitedPPoRf0[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf1(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf1[lab->getStamp().get()] = NodeStatus::entered;
	if (true && genmc::isa<WriteLabel>(lab) &&
	    ((genmc::isa<ReadLabel>(lab) && genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
	     (genmc::isa<WriteLabel>(lab) && genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<ReadLabel>(pLab) &&
			    ((genmc::isa<ReadLabel>(pLab) &&
			      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
			     (genmc::isa<WriteLabel>(pLab) &&
			      genmc::dyn_cast<WriteLabel>(pLab)->isRMW())))
				if (pporf.updateIdx(pLab->getPos()); true) {
					auto status = visitedPPoRf0[pLab->getStamp().get()];
					if (status == NodeStatus::unseen)
						visitPPoRf0(pLab, pporf);
				}
	if (auto pLab = g.tc_pred(lab); pLab) {
		auto status = visitedPPoRf1[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf1(pLab, pporf);
	}
	if (auto pLab = g.tj_pred(lab); pLab) {
		auto status = visitedPPoRf1[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf1(pLab, pporf);
	}
	for (auto &tmp : g.lin_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	if (auto pLab = g.rfe_pred(lab); pLab) {
		auto status = visitedPPoRf1[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf1(pLab, pporf);
	}
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.ctrl_preds(lab))
			if (auto *pLab = &tmp; true) {
				auto status = visitedPPoRf1[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf1(pLab, pporf);
			}
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.addr_preds(lab))
			if (auto *pLab = &tmp; true) {
				auto status = visitedPPoRf1[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf1(pLab, pporf);
			}
	if (true && genmc::isa<ReadLabel>(lab))
		for (auto &tmp : g.addr_preds(lab))
			if (auto *pLab = &tmp; true) {
				auto status = visitedPPoRf1[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf1(pLab, pporf);
			}
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.data_preds(lab))
			if (auto *pLab = &tmp; true) {
				auto status = visitedPPoRf1[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf1(pLab, pporf);
			}
	for (auto &tmp : g.moi_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	for (auto &tmp : g.fri_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	if (true && genmc::isa<WriteLabel>(lab) && lab->isAtLeastRelease())
		if (auto pLab = g.po_imm_pred(lab); pLab) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb())
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<WriteLabel>(pLab)) {
				auto status = visitedPPoRf1[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf1(pLab, pporf);
			}
	if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb())
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<ReadLabel>(pLab)) {
				auto status = visitedPPoRf1[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf1(pLab, pporf);
			}
	if (true && genmc::isa<WriteLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb()) {
				auto status = visitedPPoRf1[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf1(pLab, pporf);
			}
	if (true && genmc::isa<WriteLabel>(lab) &&
	    ((genmc::isa<ReadLabel>(lab) && genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
	     (genmc::isa<WriteLabel>(lab) && genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<ReadLabel>(pLab) &&
			    ((genmc::isa<ReadLabel>(pLab) &&
			      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
			     (genmc::isa<WriteLabel>(pLab) &&
			      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
				auto status = visitedPPoRf1[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf1(pLab, pporf);
			}
	if (true && genmc::isa<ReadLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb()) {
				auto status = visitedPPoRf1[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf1(pLab, pporf);
			}
	if (true && genmc::isa<ReadLabel>(lab))
		for (auto &tmp : g.addr_preds(lab))
			if (auto *pLab = &tmp; true)
				if (pporf.updateIdx(pLab->getPos()); true) {
					auto status = visitedPPoRf2[pLab->getStamp().get()];
					if (status == NodeStatus::unseen)
						visitPPoRf2(pLab, pporf);
				}
	if (auto pLab = g.rfi_pred(lab); pLab)
		if (true && !(pLab->isNotAtomic())) {
			auto status = visitedPPoRf5[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf5(pLab, pporf);
		}
	if (auto pLab = g.rfi_pred(lab); pLab)
		if (true && !(pLab->isNotAtomic()) && genmc::isa<WriteLabel>(pLab) &&
		    ((genmc::isa<ReadLabel>(pLab) && genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
		     (genmc::isa<WriteLabel>(pLab) &&
		      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
			auto status = visitedPPoRf6[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf6(pLab, pporf);
		}
	if (auto pLab = g.tc_pred(lab); pLab)
		if (pporf.updateIdx(pLab->getPos()); true) {
			auto status = visitedPPoRf7[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf7(pLab, pporf);
		}
	if (auto pLab = g.tj_pred(lab); pLab)
		if (pporf.updateIdx(pLab->getPos()); true) {
			auto status = visitedPPoRf7[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf7(pLab, pporf);
		}
	for (auto &tmp : g.lin_preds(lab))
		if (auto *pLab = &tmp; true)
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf7[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf7(pLab, pporf);
			}
	if (auto pLab = g.rfe_pred(lab); pLab)
		if (pporf.updateIdx(pLab->getPos()); true) {
			auto status = visitedPPoRf7[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf7(pLab, pporf);
		}
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.ctrl_preds(lab))
			if (auto *pLab = &tmp; true)
				if (pporf.updateIdx(pLab->getPos()); true) {
					auto status = visitedPPoRf7[pLab->getStamp().get()];
					if (status == NodeStatus::unseen)
						visitPPoRf7(pLab, pporf);
				}
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.addr_preds(lab))
			if (auto *pLab = &tmp; true)
				if (pporf.updateIdx(pLab->getPos()); true) {
					auto status = visitedPPoRf7[pLab->getStamp().get()];
					if (status == NodeStatus::unseen)
						visitPPoRf7(pLab, pporf);
				}
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.data_preds(lab))
			if (auto *pLab = &tmp; true)
				if (pporf.updateIdx(pLab->getPos()); true) {
					auto status = visitedPPoRf7[pLab->getStamp().get()];
					if (status == NodeStatus::unseen)
						visitPPoRf7(pLab, pporf);
				}
	for (auto &tmp : g.moi_preds(lab))
		if (auto *pLab = &tmp; true)
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf7[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf7(pLab, pporf);
			}
	for (auto &tmp : g.fri_preds(lab))
		if (auto *pLab = &tmp; true)
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf7[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf7(pLab, pporf);
			}
	if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb())
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<WriteLabel>(pLab))
				if (pporf.updateIdx(pLab->getPos()); true) {
					auto status = visitedPPoRf7[pLab->getStamp().get()];
					if (status == NodeStatus::unseen)
						visitPPoRf7(pLab, pporf);
				}
	if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb())
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<ReadLabel>(pLab))
				if (pporf.updateIdx(pLab->getPos()); true) {
					auto status = visitedPPoRf7[pLab->getStamp().get()];
					if (status == NodeStatus::unseen)
						visitPPoRf7(pLab, pporf);
				}
	if (true && genmc::isa<WriteLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb())
				if (pporf.updateIdx(pLab->getPos()); true) {
					auto status = visitedPPoRf7[pLab->getStamp().get()];
					if (status == NodeStatus::unseen)
						visitPPoRf7(pLab, pporf);
				}
	if (true && genmc::isa<ReadLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb())
				if (pporf.updateIdx(pLab->getPos()); true) {
					auto status = visitedPPoRf7[pLab->getStamp().get()];
					if (status == NodeStatus::unseen)
						visitPPoRf7(pLab, pporf);
				}
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.ctrl_preds(lab))
			if (auto *pLab = &tmp; true) {
				auto status = visitedPPoRf10[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf10(pLab, pporf);
			}
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.addr_preds(lab))
			if (auto *pLab = &tmp; true) {
				auto status = visitedPPoRf10[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf10(pLab, pporf);
			}
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.data_preds(lab))
			if (auto *pLab = &tmp; true) {
				auto status = visitedPPoRf10[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf10(pLab, pporf);
			}
	if (true && genmc::isa<WriteLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
			    static_cast<const LkmmFenceLabel *>(
				    static_cast<const EventLabel *>(pLab))
					    ->getFenceKind() == LkmmFenceKind::Wmb) {
				auto status = visitedPPoRf12[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf12(pLab, pporf);
			}
	if (true && genmc::isa<WriteLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab) {
			auto status = visitedPPoRf13[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf13(pLab, pporf);
		}
	if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb())
		if (auto pLab = g.po_imm_pred(lab); pLab) {
			auto status = visitedPPoRf14[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf14(pLab, pporf);
		}
	if (true && genmc::isa<WriteLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
			    static_cast<const LkmmFenceLabel *>(
				    static_cast<const EventLabel *>(pLab))
					    ->getFenceKind() == LkmmFenceKind::Mb) {
				auto status = visitedPPoRf14[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf14(pLab, pporf);
			}
	if (true && genmc::isa<WriteLabel>(lab) &&
	    ((genmc::isa<ReadLabel>(lab) && genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
	     (genmc::isa<WriteLabel>(lab) && genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
			    static_cast<const LkmmFenceLabel *>(
				    static_cast<const EventLabel *>(pLab))
					    ->getFenceKind() == LkmmFenceKind::BeforeAtomic) {
				auto status = visitedPPoRf14[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf14(pLab, pporf);
			}
	if (true && genmc::isa<ReadLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
			    static_cast<const LkmmFenceLabel *>(
				    static_cast<const EventLabel *>(pLab))
					    ->getFenceKind() == LkmmFenceKind::Mb) {
				auto status = visitedPPoRf14[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf14(pLab, pporf);
			}
	if (true && genmc::isa<ReadLabel>(lab) &&
	    ((genmc::isa<ReadLabel>(lab) && genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
	     (genmc::isa<WriteLabel>(lab) && genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
			    static_cast<const LkmmFenceLabel *>(
				    static_cast<const EventLabel *>(pLab))
					    ->getFenceKind() == LkmmFenceKind::BeforeAtomic) {
				auto status = visitedPPoRf14[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf14(pLab, pporf);
			}
	if (true && genmc::isa<WriteLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab) {
			auto status = visitedPPoRf15[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf15(pLab, pporf);
		}
	if (true && genmc::isa<ReadLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab) {
			auto status = visitedPPoRf15[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf15(pLab, pporf);
		}
	if (true && genmc::isa<WriteLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
				auto status = visitedPPoRf16[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf16(pLab, pporf);
			}
	if (true && genmc::isa<WriteLabel>(lab) &&
	    ((genmc::isa<ReadLabel>(lab) && genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
	     (genmc::isa<WriteLabel>(lab) && genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
		if (auto pLab = g.po_imm_pred(lab); pLab) {
			auto status = visitedPPoRf16[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf16(pLab, pporf);
		}
	if (true && genmc::isa<ReadLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
				auto status = visitedPPoRf16[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf16(pLab, pporf);
			}
	if (true && genmc::isa<ReadLabel>(lab) &&
	    ((genmc::isa<ReadLabel>(lab) && genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
	     (genmc::isa<WriteLabel>(lab) && genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
		if (auto pLab = g.po_imm_pred(lab); pLab) {
			auto status = visitedPPoRf16[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf16(pLab, pporf);
		}
	if (true && genmc::isa<WriteLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
			    static_cast<const LkmmFenceLabel *>(
				    static_cast<const EventLabel *>(pLab))
					    ->getFenceKind() == LkmmFenceKind::AfterAtomic) {
				auto status = visitedPPoRf17[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf17(pLab, pporf);
			}
	if (true && genmc::isa<ReadLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
			    static_cast<const LkmmFenceLabel *>(
				    static_cast<const EventLabel *>(pLab))
					    ->getFenceKind() == LkmmFenceKind::AfterAtomic) {
				auto status = visitedPPoRf17[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf17(pLab, pporf);
			}
	if (true && genmc::isa<WriteLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
			    static_cast<const LkmmFenceLabel *>(
				    static_cast<const EventLabel *>(pLab))
					    ->getFenceKind() == LkmmFenceKind::AfterSpinlock) {
				auto status = visitedPPoRf18[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf18(pLab, pporf);
			}
	if (true && genmc::isa<ReadLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
			    static_cast<const LkmmFenceLabel *>(
				    static_cast<const EventLabel *>(pLab))
					    ->getFenceKind() == LkmmFenceKind::AfterSpinlock) {
				auto status = visitedPPoRf18[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf18(pLab, pporf);
			}
	if (true && genmc::isa<WriteLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
			    static_cast<const LkmmFenceLabel *>(
				    static_cast<const EventLabel *>(pLab))
					    ->getFenceKind() == LkmmFenceKind::AfterUnlockLock) {
				auto status = visitedPPoRf21[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf21(pLab, pporf);
			}
	if (true && genmc::isa<ReadLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
			    static_cast<const LkmmFenceLabel *>(
				    static_cast<const EventLabel *>(pLab))
					    ->getFenceKind() == LkmmFenceKind::AfterUnlockLock) {
				auto status = visitedPPoRf21[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf21(pLab, pporf);
			}
	if (true && genmc::isa<WriteLabel>(lab) && lab->isAtLeastRelease())
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf22[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf22(pLab, pporf);
			}
	if (auto pLab = g.tc_pred(lab); pLab) {
		auto status = visitedPPoRf23[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf23(pLab, pporf);
	}
	if (auto pLab = g.tj_pred(lab); pLab) {
		auto status = visitedPPoRf23[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf23(pLab, pporf);
	}
	for (auto &tmp : g.lin_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	if (auto pLab = g.rfe_pred(lab); pLab) {
		auto status = visitedPPoRf23[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf23(pLab, pporf);
	}
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.ctrl_preds(lab))
			if (auto *pLab = &tmp; true) {
				auto status = visitedPPoRf23[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf23(pLab, pporf);
			}
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.addr_preds(lab))
			if (auto *pLab = &tmp; true) {
				auto status = visitedPPoRf23[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf23(pLab, pporf);
			}
	if (true && genmc::isa<ReadLabel>(lab))
		for (auto &tmp : g.addr_preds(lab))
			if (auto *pLab = &tmp; true) {
				auto status = visitedPPoRf23[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf23(pLab, pporf);
			}
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.data_preds(lab))
			if (auto *pLab = &tmp; true) {
				auto status = visitedPPoRf23[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf23(pLab, pporf);
			}
	for (auto &tmp : g.moi_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	for (auto &tmp : g.fri_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	if (true && genmc::isa<WriteLabel>(lab) && lab->isAtLeastRelease())
		if (auto pLab = g.po_imm_pred(lab); pLab) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb())
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<WriteLabel>(pLab)) {
				auto status = visitedPPoRf23[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf23(pLab, pporf);
			}
	if (true && genmc::isa<ReadLabel>(lab) && lab->isLkmmMb())
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<ReadLabel>(pLab)) {
				auto status = visitedPPoRf23[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf23(pLab, pporf);
			}
	if (true && genmc::isa<WriteLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb()) {
				auto status = visitedPPoRf23[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf23(pLab, pporf);
			}
	if (true && genmc::isa<WriteLabel>(lab) &&
	    ((genmc::isa<ReadLabel>(lab) && genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
	     (genmc::isa<WriteLabel>(lab) && genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<ReadLabel>(pLab) &&
			    ((genmc::isa<ReadLabel>(pLab) &&
			      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
			     (genmc::isa<WriteLabel>(pLab) &&
			      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
				auto status = visitedPPoRf23[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf23(pLab, pporf);
			}
	if (true && genmc::isa<ReadLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb()) {
				auto status = visitedPPoRf23[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf23(pLab, pporf);
			}
	if (true && genmc::isa<ReadLabel>(lab) && !lab->isLkmmNoreturn())
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
			    static_cast<const LkmmFenceLabel *>(
				    static_cast<const EventLabel *>(pLab))
					    ->getFenceKind() == LkmmFenceKind::Rmb) {
				auto status = visitedPPoRf24[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf24(pLab, pporf);
			}
	if (true && genmc::isa<ReadLabel>(lab) && !lab->isLkmmNoreturn())
		if (auto pLab = g.po_imm_pred(lab); pLab) {
			auto status = visitedPPoRf25[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf25(pLab, pporf);
		}
	visitedPPoRf1[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf2(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf2[lab->getStamp().get()] = NodeStatus::entered;
	pporf.updateIdx(lab->getPos());
	if (auto pLab = g.rfi_pred(lab); pLab) {
		auto status = visitedPPoRf3[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf3(pLab, pporf);
	}
	if (auto pLab = g.rfi_pred(lab); pLab)
		if (true && genmc::isa<WriteLabel>(pLab) &&
		    ((genmc::isa<ReadLabel>(pLab) && genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
		     (genmc::isa<WriteLabel>(pLab) &&
		      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
			auto status = visitedPPoRf4[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf4(pLab, pporf);
		}
	visitedPPoRf2[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf3(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf3[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &tmp : g.data_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	for (auto &tmp : g.data_preds(lab))
		if (auto *pLab = &tmp; true)
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf2[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf2(pLab, pporf);
			}
	for (auto &tmp : g.data_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	visitedPPoRf3[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf4(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf4[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<ReadLabel>(pLab) &&
		    ((genmc::isa<ReadLabel>(pLab) && genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
		     (genmc::isa<WriteLabel>(pLab) &&
		      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
			auto status = visitedPPoRf3[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf3(pLab, pporf);
		}
	visitedPPoRf4[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf5(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf5[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &tmp : g.addr_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	for (auto &tmp : g.data_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	for (auto &tmp : g.addr_preds(lab))
		if (auto *pLab = &tmp; true)
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf2[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf2(pLab, pporf);
			}
	for (auto &tmp : g.data_preds(lab))
		if (auto *pLab = &tmp; true)
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf2[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf2(pLab, pporf);
			}
	for (auto &tmp : g.addr_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	for (auto &tmp : g.data_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	visitedPPoRf5[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf6(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf6[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<ReadLabel>(pLab) &&
		    ((genmc::isa<ReadLabel>(pLab) && genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
		     (genmc::isa<WriteLabel>(pLab) &&
		      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
			auto status = visitedPPoRf5[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf5(pLab, pporf);
		}
	visitedPPoRf6[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf7(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf7[lab->getStamp().get()] = NodeStatus::entered;
	pporf.updateIdx(lab->getPos());
	visitedPPoRf7[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf8(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf8[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &tmp : g.data_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	for (auto &tmp : g.data_preds(lab))
		if (auto *pLab = &tmp; true)
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf7[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf7(pLab, pporf);
			}
	for (auto &tmp : g.data_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf10[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf10(pLab, pporf);
		}
	for (auto &tmp : g.data_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	visitedPPoRf8[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf9(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf9[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<ReadLabel>(pLab) &&
		    ((genmc::isa<ReadLabel>(pLab) && genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
		     (genmc::isa<WriteLabel>(pLab) &&
		      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
			auto status = visitedPPoRf8[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf8(pLab, pporf);
		}
	visitedPPoRf9[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf10(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf10[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.rfi_pred(lab); pLab) {
		auto status = visitedPPoRf8[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf8(pLab, pporf);
	}
	if (auto pLab = g.rfi_pred(lab); pLab)
		if (true && genmc::isa<WriteLabel>(pLab) &&
		    ((genmc::isa<ReadLabel>(pLab) && genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
		     (genmc::isa<WriteLabel>(pLab) &&
		      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
			auto status = visitedPPoRf9[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf9(pLab, pporf);
		}
	visitedPPoRf10[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf11(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf11[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &tmp : g.addr_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	for (auto &tmp : g.addr_preds(lab))
		if (auto *pLab = &tmp; true)
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf2[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf2(pLab, pporf);
			}
	for (auto &tmp : g.addr_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	visitedPPoRf11[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf12(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf12[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<WriteLabel>(pLab)) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<WriteLabel>(pLab))
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf7[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf7(pLab, pporf);
			}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && pLab->isNotAtomic() && genmc::isa<WriteLabel>(pLab)) {
			auto status = visitedPPoRf11[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf11(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab) {
		auto status = visitedPPoRf12[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf12(pLab, pporf);
	}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<WriteLabel>(pLab)) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	visitedPPoRf12[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf13(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf13[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
		    static_cast<const LkmmFenceLabel *>(static_cast<const EventLabel *>(pLab))
				    ->getFenceKind() == LkmmFenceKind::Wmb) {
			auto status = visitedPPoRf12[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf12(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab) {
		auto status = visitedPPoRf13[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf13(pLab, pporf);
	}
	visitedPPoRf13[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf14(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf14[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<WriteLabel>(pLab)) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<ReadLabel>(pLab)) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<WriteLabel>(pLab))
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf7[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf7(pLab, pporf);
			}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<ReadLabel>(pLab))
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf7[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf7(pLab, pporf);
			}
	if (auto pLab = g.po_imm_pred(lab); pLab) {
		auto status = visitedPPoRf14[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf14(pLab, pporf);
	}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<WriteLabel>(pLab)) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<ReadLabel>(pLab)) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	visitedPPoRf14[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf15(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf15[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb()) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb())
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf7[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf7(pLab, pporf);
			}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
		    static_cast<const LkmmFenceLabel *>(static_cast<const EventLabel *>(pLab))
				    ->getFenceKind() == LkmmFenceKind::Mb) {
			auto status = visitedPPoRf14[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf14(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab) {
		auto status = visitedPPoRf15[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf15(pLab, pporf);
	}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true &&
		    ((genmc::isa<ReadLabel>(pLab) && genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
		     (genmc::isa<WriteLabel>(pLab) &&
		      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
			auto status = visitedPPoRf16[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf16(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
		    static_cast<const LkmmFenceLabel *>(static_cast<const EventLabel *>(pLab))
				    ->getFenceKind() == LkmmFenceKind::AfterAtomic) {
			auto status = visitedPPoRf17[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf17(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
		    static_cast<const LkmmFenceLabel *>(static_cast<const EventLabel *>(pLab))
				    ->getFenceKind() == LkmmFenceKind::AfterSpinlock) {
			auto status = visitedPPoRf18[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf18(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
		    static_cast<const LkmmFenceLabel *>(static_cast<const EventLabel *>(pLab))
				    ->getFenceKind() == LkmmFenceKind::AfterUnlockLock) {
			auto status = visitedPPoRf21[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf21(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<WriteLabel>(pLab) && pLab->isLkmmMb()) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	visitedPPoRf15[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf16(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf16[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
		    static_cast<const LkmmFenceLabel *>(static_cast<const EventLabel *>(pLab))
				    ->getFenceKind() == LkmmFenceKind::BeforeAtomic) {
			auto status = visitedPPoRf14[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf14(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab) {
		auto status = visitedPPoRf16[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf16(pLab, pporf);
	}
	visitedPPoRf16[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf17(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf17[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<WriteLabel>(pLab) &&
		    ((genmc::isa<ReadLabel>(pLab) && genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
		     (genmc::isa<WriteLabel>(pLab) &&
		      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<ReadLabel>(pLab) &&
		    ((genmc::isa<ReadLabel>(pLab) && genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
		     (genmc::isa<WriteLabel>(pLab) &&
		      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<WriteLabel>(pLab) &&
		    ((genmc::isa<ReadLabel>(pLab) && genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
		     (genmc::isa<WriteLabel>(pLab) && genmc::dyn_cast<WriteLabel>(pLab)->isRMW())))
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf7[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf7(pLab, pporf);
			}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<ReadLabel>(pLab) &&
		    ((genmc::isa<ReadLabel>(pLab) && genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
		     (genmc::isa<WriteLabel>(pLab) && genmc::dyn_cast<WriteLabel>(pLab)->isRMW())))
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf7[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf7(pLab, pporf);
			}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true &&
		    ((genmc::isa<ReadLabel>(pLab) && genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
		     (genmc::isa<WriteLabel>(pLab) &&
		      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
			auto status = visitedPPoRf14[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf14(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab) {
		auto status = visitedPPoRf17[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf17(pLab, pporf);
	}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<WriteLabel>(pLab) &&
		    ((genmc::isa<ReadLabel>(pLab) && genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
		     (genmc::isa<WriteLabel>(pLab) &&
		      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<ReadLabel>(pLab) &&
		    ((genmc::isa<ReadLabel>(pLab) && genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
		     (genmc::isa<WriteLabel>(pLab) &&
		      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	visitedPPoRf17[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf18(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf18[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
		    genmc::isa<TrylockCasWriteLabel>(pLab) && genmc::isa<WriteLabel>(pLab)) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
		    genmc::isa<TrylockCasWriteLabel>(pLab) && genmc::isa<ReadLabel>(pLab)) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
		    genmc::isa<TrylockCasWriteLabel>(pLab) && genmc::isa<WriteLabel>(pLab))
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf7[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf7(pLab, pporf);
			}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
		    genmc::isa<TrylockCasWriteLabel>(pLab) && genmc::isa<ReadLabel>(pLab))
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf7[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf7(pLab, pporf);
			}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
		    genmc::isa<TrylockCasWriteLabel>(pLab)) {
			auto status = visitedPPoRf14[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf14(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab) {
		auto status = visitedPPoRf18[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf18(pLab, pporf);
	}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
		    genmc::isa<TrylockCasWriteLabel>(pLab) && genmc::isa<WriteLabel>(pLab)) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<LockCasWriteLabel>(pLab) ||
		    genmc::isa<TrylockCasWriteLabel>(pLab) && genmc::isa<ReadLabel>(pLab)) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	visitedPPoRf18[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf19(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf19[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<UnlockWriteLabel>(pLab)) {
			auto status = visitedPPoRf14[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf14(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab) {
		auto status = visitedPPoRf19[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf19(pLab, pporf);
	}
	visitedPPoRf19[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf20(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf20[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.rfi_pred(lab); pLab)
		if (true && genmc::isa<UnlockWriteLabel>(pLab)) {
			auto status = visitedPPoRf14[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf14(pLab, pporf);
		}
	visitedPPoRf20[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf21(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf21[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true &&
		    (genmc::isa<LockCasReadLabel>(pLab) || genmc::isa<TrylockCasReadLabel>(pLab)) &&
		    static_cast<const ReadLabel *>(static_cast<const EventLabel *>(pLab))
			    ->isRMW()) {
			auto status = visitedPPoRf19[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf19(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true &&
		    (genmc::isa<LockCasReadLabel>(pLab) || genmc::isa<TrylockCasReadLabel>(pLab)) &&
		    static_cast<const ReadLabel *>(static_cast<const EventLabel *>(pLab))
			    ->isRMW()) {
			auto status = visitedPPoRf20[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf20(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab) {
		auto status = visitedPPoRf21[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf21(pLab, pporf);
	}
	visitedPPoRf21[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf22(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf22[lab->getStamp().get()] = NodeStatus::entered;
	pporf.updateIdx(lab->getPos());
	if (auto pLab = g.po_imm_pred(lab); pLab) {
		auto status = visitedPPoRf1[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf1(pLab, pporf);
	}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (pporf.updateIdx(pLab->getPos()); true) {
			auto status = visitedPPoRf22[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf22(pLab, pporf);
		}
	visitedPPoRf22[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf23(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf23[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<ReadLabel>(pLab) && pLab->isAtLeastAcquire() &&
		    (!genmc::isa<CasReadLabel>(pLab) ||
		     static_cast<const ReadLabel *>(static_cast<const EventLabel *>(pLab))
			     ->isRMW())) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<ReadLabel>(pLab) && pLab->isAtLeastAcquire() &&
		    (!genmc::isa<CasReadLabel>(pLab) ||
		     static_cast<const ReadLabel *>(static_cast<const EventLabel *>(pLab))
			     ->isRMW()))
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf7[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf7(pLab, pporf);
			}
	if (auto pLab = g.po_imm_pred(lab); pLab) {
		auto status = visitedPPoRf23[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf23(pLab, pporf);
	}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<ReadLabel>(pLab) && pLab->isAtLeastAcquire() &&
		    (!genmc::isa<CasReadLabel>(pLab) ||
		     static_cast<const ReadLabel *>(static_cast<const EventLabel *>(pLab))
			     ->isRMW())) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true &&
		    (genmc::isa<LockCasReadLabel>(pLab) || genmc::isa<TrylockCasReadLabel>(pLab)) &&
		    static_cast<const ReadLabel *>(static_cast<const EventLabel *>(pLab))
			    ->isRMW()) {
			auto status = visitedPPoRf27[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf27(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true &&
		    (genmc::isa<LockCasReadLabel>(pLab) || genmc::isa<TrylockCasReadLabel>(pLab)) &&
		    static_cast<const ReadLabel *>(static_cast<const EventLabel *>(pLab))
			    ->isRMW()) {
			auto status = visitedPPoRf28[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf28(pLab, pporf);
		}
	visitedPPoRf23[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf24(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf24[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<ReadLabel>(pLab) && !pLab->isLkmmNoreturn()) {
			auto status = visitedPPoRf1[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf1(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<ReadLabel>(pLab) && !pLab->isLkmmNoreturn())
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf7[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf7(pLab, pporf);
			}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<ReadLabel>(pLab) && !pLab->isLkmmNoreturn()) {
			auto status = visitedPPoRf23[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf23(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab) {
		auto status = visitedPPoRf24[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf24(pLab, pporf);
	}
	visitedPPoRf24[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf25(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf25[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<LkmmFenceLabel>(pLab) &&
		    static_cast<const LkmmFenceLabel *>(static_cast<const EventLabel *>(pLab))
				    ->getFenceKind() == LkmmFenceKind::Rmb) {
			auto status = visitedPPoRf24[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf24(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab) {
		auto status = visitedPPoRf25[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf25(pLab, pporf);
	}
	visitedPPoRf25[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf26(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf26[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.po_imm_pred(lab); pLab) {
		auto status = visitedPPoRf1[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf1(pLab, pporf);
	}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (pporf.updateIdx(pLab->getPos()); true) {
			auto status = visitedPPoRf7[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf7(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab) {
		auto status = visitedPPoRf23[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf23(pLab, pporf);
	}
	if (auto pLab = g.po_imm_pred(lab); pLab) {
		auto status = visitedPPoRf26[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf26(pLab, pporf);
	}
	visitedPPoRf26[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf27(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf27[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<UnlockWriteLabel>(pLab)) {
			auto status = visitedPPoRf26[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf26(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab) {
		auto status = visitedPPoRf27[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf27(pLab, pporf);
	}
	visitedPPoRf27[lab->getStamp().get()] = NodeStatus::left;
}

void LKMMChecker::visitPPoRf28(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf28[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.rfi_pred(lab); pLab)
		if (true && genmc::isa<UnlockWriteLabel>(pLab)) {
			auto status = visitedPPoRf26[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf26(pLab, pporf);
		}
	visitedPPoRf28[lab->getStamp().get()] = NodeStatus::left;
}

DepView LKMMChecker::calcPPoRfBefore(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();
	DepView pporf;
	pporf.updateIdx(lab->getPos());
	visitedPPoRf0.clear();
	visitedPPoRf0.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf1.clear();
	visitedPPoRf1.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf2.clear();
	visitedPPoRf2.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf3.clear();
	visitedPPoRf3.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf4.clear();
	visitedPPoRf4.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf5.clear();
	visitedPPoRf5.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf6.clear();
	visitedPPoRf6.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf7.clear();
	visitedPPoRf7.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf8.clear();
	visitedPPoRf8.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf9.clear();
	visitedPPoRf9.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf10.clear();
	visitedPPoRf10.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf11.clear();
	visitedPPoRf11.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf12.clear();
	visitedPPoRf12.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf13.clear();
	visitedPPoRf13.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf14.clear();
	visitedPPoRf14.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf15.clear();
	visitedPPoRf15.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf16.clear();
	visitedPPoRf16.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf17.clear();
	visitedPPoRf17.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf18.clear();
	visitedPPoRf18.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf19.clear();
	visitedPPoRf19.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf20.clear();
	visitedPPoRf20.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf21.clear();
	visitedPPoRf21.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf22.clear();
	visitedPPoRf22.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf23.clear();
	visitedPPoRf23.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf24.clear();
	visitedPPoRf24.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf25.clear();
	visitedPPoRf25.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf26.clear();
	visitedPPoRf26.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf27.clear();
	visitedPPoRf27.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);
	visitedPPoRf28.clear();
	visitedPPoRf28.resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);

	visitPPoRf1(lab, pporf);
	visitPPoRf23(lab, pporf);
	return pporf;
}

std::unique_ptr<VectorClock> LKMMChecker::calculatePrefixView(const EventLabel *lab) const
{
	return std::make_unique<DepView>(calcPPoRfBefore(lab));
}

void LKMMChecker::recomputeCacheCounters([[maybe_unused]] const ExecutionGraph &g) const {}

void LKMMChecker::resetCacheCounters() const {}

void LKMMChecker::maybeDecreaseCacheCounters([[maybe_unused]] const EventLabel *lab) const {}

void LKMMChecker::maybeIncreaseCacheCounters([[maybe_unused]] const EventLabel *lab) const {}

// NOLINTEND
