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
 * From kat/imm-genmc.kat, Kater v0.4.0-vzhn.3-25-g0c366e6 (scripts/lkmm/kater-export.sh).
 *******************************************************************************/

// NOLINTBEGIN
#include "IMMChecker.hpp"
#include "genmc/ADT/DepView.hpp"
#include "genmc/ADT/VSet.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Execution/GraphUtils.hpp"
#include "genmc/Support/VerificationError.hpp"

#include <algorithm>

bool IMMChecker::isDepTracking() const { return 1; }

bool IMMChecker::visitCalc76Iterative(std::vector<DFSWorklistEntry> &worklist, View &calcRes) const
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
			if (true && lab->isAtLeastAcquire())
				if (auto pLab = g.rf_pred(lab); pLab)
					if (true && pLab->isAtLeastRelease())
						if (calcRes.updateIdx(pLab->getPos()); true) {
							if (!genmc::isa<InitLabel>(pLab)) {
								worklist.emplace_back(0, pLab);
							}
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
			if (true && lab->isAtLeastAcquire())
				if (auto *iLab =
					    genmc::dyn_cast_if_present<InitLabel>(g.rf_pred(lab));
				    iLab) {
					calcRes.update(iLab->locView(
						genmc::cast<ReadLabel>(lab)->getAddr(), 0));
				}
			if (true && lab->isAtLeastAcquire())
				if (auto pLab = g.rf_pred(lab); pLab)
					if (true && pLab->isAtLeastRelease()) {
						if (!genmc::isa<InitLabel>(pLab)) {
							worklist.emplace_back(1, pLab);
						}
					}
			if (true && lab->isAtLeastAcquire())
				if (auto *iLab =
					    genmc::dyn_cast_if_present<InitLabel>(g.rf_pred(lab));
				    iLab) {
					calcRes.update(iLab->locView(
						genmc::cast<ReadLabel>(lab)->getAddr(), 0));
				}
			if (true && lab->isAtLeastAcquire())
				if (auto pLab = g.rf_pred(lab); pLab) {
					if (!genmc::isa<InitLabel>(pLab)) {
						auto status = visitedCalc76_3.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(3, pLab);
						}
					}
				}
			if (true && lab->isAtLeastAcquire())
				if (auto *iLab =
					    genmc::dyn_cast_if_present<InitLabel>(g.rf_pred(lab));
				    iLab) {
					calcRes.update(iLab->locView(
						genmc::cast<ReadLabel>(lab)->getAddr(), 0));
				}
			if (true && lab->isAtLeastAcquire())
				if (auto pLab = g.rf_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						if (!genmc::isa<InitLabel>(pLab)) {
							auto status = visitedCalc76_5.getStatus(
								pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(5, pLab);
							}
						}
					}
			if (true && lab->isAtLeastAcquire() && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status =
						visitedCalc76_6.getStatus(pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
					}
				}
			if (true && lab->isAtLeastAcquire() && genmc::isa<ThreadJoinLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status =
						visitedCalc76_6.getStatus(pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
					}
				}

			break;
		}
		case 3: {
			if (isFinishing) {
				visitedCalc76_3.setStatus(lab->getStamp().get(), NodeStatus::left);
				break;
			}

			auto status = visitedCalc76_3.getStatus(lab->getStamp().get());
			if (status != NodeStatus::unseen)
				break; /* already explored */

			worklist.emplace_back(3, lab, true);
			visitedCalc76_3.setStatus(lab->getStamp().get(), NodeStatus::entered);

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab))
					if (calcRes.updateIdx(pLab->getPos()); true) {
						worklist.emplace_back(0, pLab);
					}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<ThreadCreateLabel>(pLab))
					if (calcRes.updateIdx(pLab->getPos()); true) {
						worklist.emplace_back(0, pLab);
					}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					worklist.emplace_back(1, pLab);
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<ThreadCreateLabel>(pLab)) {
					worklist.emplace_back(1, pLab);
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status = visitedCalc76_3.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(3, pLab);
				}
			}

			break;
		}
		case 4: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease())
					if (calcRes.updateIdx(pLab->getPos()); true) {
						if (!genmc::isa<InitLabel>(pLab)) {
							worklist.emplace_back(0, pLab);
						}
					}
			if (auto *iLab = genmc::dyn_cast_if_present<InitLabel>(g.rf_pred(lab));
			    iLab) {
				calcRes.update(
					iLab->locView(genmc::cast<ReadLabel>(lab)->getAddr(), 0));
			}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease()) {
					if (!genmc::isa<InitLabel>(pLab)) {
						worklist.emplace_back(1, pLab);
					}
				}
			if (auto *iLab = genmc::dyn_cast_if_present<InitLabel>(g.rf_pred(lab));
			    iLab) {
				calcRes.update(
					iLab->locView(genmc::cast<ReadLabel>(lab)->getAddr(), 0));
			}
			if (auto pLab = g.rf_pred(lab); pLab) {
				if (!genmc::isa<InitLabel>(pLab)) {
					auto status =
						visitedCalc76_3.getStatus(pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					}
				}
			}
			if (auto *iLab = genmc::dyn_cast_if_present<InitLabel>(g.rf_pred(lab));
			    iLab) {
				calcRes.update(
					iLab->locView(genmc::cast<ReadLabel>(lab)->getAddr(), 0));
			}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					if (!genmc::isa<InitLabel>(pLab)) {
						auto status = visitedCalc76_5.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(5, pLab);
						}
					}
				}

			break;
		}
		case 5: {
			if (isFinishing) {
				visitedCalc76_5.setStatus(lab->getStamp().get(), NodeStatus::left);
				break;
			}

			auto status = visitedCalc76_5.getStatus(lab->getStamp().get());
			if (status != NodeStatus::unseen)
				break; /* already explored */

			worklist.emplace_back(5, lab, true);
			visitedCalc76_5.setStatus(lab->getStamp().get(), NodeStatus::entered);

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					worklist.emplace_back(4, pLab);
				}

			break;
		}
		case 6: {
			if (isFinishing) {
				visitedCalc76_6.setStatus(lab->getStamp().get(), NodeStatus::left);
				break;
			}

			auto status = visitedCalc76_6.getStatus(lab->getStamp().get());
			if (status != NodeStatus::unseen)
				break; /* already explored */

			worklist.emplace_back(6, lab, true);
			visitedCalc76_6.setStatus(lab->getStamp().get(), NodeStatus::entered);

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease())
					if (calcRes.updateIdx(pLab->getPos()); true) {
						if (!genmc::isa<InitLabel>(pLab)) {
							worklist.emplace_back(0, pLab);
						}
					}
			if (auto *iLab = genmc::dyn_cast_if_present<InitLabel>(g.rf_pred(lab));
			    iLab) {
				calcRes.update(
					iLab->locView(genmc::cast<ReadLabel>(lab)->getAddr(), 0));
			}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease()) {
					if (!genmc::isa<InitLabel>(pLab)) {
						worklist.emplace_back(1, pLab);
					}
				}
			if (auto *iLab = genmc::dyn_cast_if_present<InitLabel>(g.rf_pred(lab));
			    iLab) {
				calcRes.update(
					iLab->locView(genmc::cast<ReadLabel>(lab)->getAddr(), 0));
			}
			if (auto pLab = g.rf_pred(lab); pLab) {
				if (!genmc::isa<InitLabel>(pLab)) {
					auto status =
						visitedCalc76_3.getStatus(pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					}
				}
			}
			if (auto *iLab = genmc::dyn_cast_if_present<InitLabel>(g.rf_pred(lab));
			    iLab) {
				calcRes.update(
					iLab->locView(genmc::cast<ReadLabel>(lab)->getAddr(), 0));
			}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					if (!genmc::isa<InitLabel>(pLab)) {
						auto status = visitedCalc76_5.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(5, pLab);
						}
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status = visitedCalc76_6.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(6, pLab);
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

View IMMChecker::visitCalc76(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();
	View calcRes;

	calcRes.updateIdx(lab->getPos());
	visitedCalc76_3.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedCalc76_5.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedCalc76_6.maybeClearResize(g.getMaxStamp().get() + 1);

	/* Explore from all accepting states using DFS */
	std::vector<DFSWorklistEntry> startStates = {
		{2, lab},
	};

	visitCalc76Iterative(startStates, calcRes);
	return calcRes;
}

auto IMMChecker::checkCalc76(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	return visitCalc76(lab);
}

bool IMMChecker::visitCalc77Iterative(std::vector<DFSWorklistEntry> &worklist, View &calcRes) const
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
			if (auto pLab = g.tc_pred(lab); pLab)
				if (calcRes.updateIdx(pLab->getPos()); true) {
					worklist.emplace_back(0, pLab);
				}
			if (auto pLab = g.tj_pred(lab); pLab)
				if (calcRes.updateIdx(pLab->getPos()); true) {
					worklist.emplace_back(0, pLab);
				}
			if (true && lab->isAtLeastAcquire() &&
			    !(genmc::isa<AbstractLockCasReadLabel>(lab)))
				if (auto pLab = g.rf_pred(lab); pLab)
					if (true && pLab->isAtLeastRelease())
						if (calcRes.updateIdx(pLab->getPos()); true) {
							if (!genmc::isa<InitLabel>(pLab)) {
								worklist.emplace_back(0, pLab);
							}
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
			if (true && lab->isAtLeastAcquire() &&
			    !(genmc::isa<AbstractLockCasReadLabel>(lab)))
				if (auto *iLab =
					    genmc::dyn_cast_if_present<InitLabel>(g.rf_pred(lab));
				    iLab) {
					calcRes.update(iLab->locView(
						genmc::cast<ReadLabel>(lab)->getAddr(), 1));
				}
			if (true && lab->isAtLeastAcquire() &&
			    !(genmc::isa<AbstractLockCasReadLabel>(lab)))
				if (auto pLab = g.rf_pred(lab); pLab)
					if (true && pLab->isAtLeastRelease()) {
						if (!genmc::isa<InitLabel>(pLab)) {
							worklist.emplace_back(1, pLab);
						}
					}
			if (true && lab->isAtLeastAcquire() &&
			    !(genmc::isa<AbstractLockCasReadLabel>(lab)))
				if (auto *iLab =
					    genmc::dyn_cast_if_present<InitLabel>(g.rf_pred(lab));
				    iLab) {
					calcRes.update(iLab->locView(
						genmc::cast<ReadLabel>(lab)->getAddr(), 1));
				}
			if (true && lab->isAtLeastAcquire() &&
			    !(genmc::isa<AbstractLockCasReadLabel>(lab)))
				if (auto pLab = g.rf_pred(lab); pLab) {
					if (!genmc::isa<InitLabel>(pLab)) {
						auto status = visitedCalc77_3.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(3, pLab);
						}
					}
				}
			if (true && lab->isAtLeastAcquire() &&
			    !(genmc::isa<AbstractLockCasReadLabel>(lab)))
				if (auto *iLab =
					    genmc::dyn_cast_if_present<InitLabel>(g.rf_pred(lab));
				    iLab) {
					calcRes.update(iLab->locView(
						genmc::cast<ReadLabel>(lab)->getAddr(), 1));
				}
			if (true && lab->isAtLeastAcquire() &&
			    !(genmc::isa<AbstractLockCasReadLabel>(lab)))
				if (auto pLab = g.rf_pred(lab); pLab)
					if (true && genmc::isa<WriteLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
						if (!genmc::isa<InitLabel>(pLab)) {
							auto status = visitedCalc77_5.getStatus(
								pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(5, pLab);
							}
						}
					}
			if (true && lab->isAtLeastAcquire() && genmc::isa<FenceLabel>(lab) &&
			    !(genmc::isa<AbstractLockCasReadLabel>(lab)))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status =
						visitedCalc77_6.getStatus(pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
					}
				}
			if (true && lab->isAtLeastAcquire() && genmc::isa<ThreadJoinLabel>(lab) &&
			    !(genmc::isa<AbstractLockCasReadLabel>(lab)))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status =
						visitedCalc77_6.getStatus(pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
					}
				}
			if (true && lab->isAtLeastAcquire() && genmc::isa<ThreadStartLabel>(lab) &&
			    !(genmc::isa<AbstractLockCasReadLabel>(lab)))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status =
						visitedCalc77_6.getStatus(pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
					}
				}

			break;
		}
		case 3: {
			if (isFinishing) {
				visitedCalc77_3.setStatus(lab->getStamp().get(), NodeStatus::left);
				break;
			}

			auto status = visitedCalc77_3.getStatus(lab->getStamp().get());
			if (status != NodeStatus::unseen)
				break; /* already explored */

			worklist.emplace_back(3, lab, true);
			visitedCalc77_3.setStatus(lab->getStamp().get(), NodeStatus::entered);

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab))
					if (calcRes.updateIdx(pLab->getPos()); true) {
						worklist.emplace_back(0, pLab);
					}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<ThreadCreateLabel>(pLab))
					if (calcRes.updateIdx(pLab->getPos()); true) {
						worklist.emplace_back(0, pLab);
					}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					worklist.emplace_back(1, pLab);
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<ThreadCreateLabel>(pLab)) {
					worklist.emplace_back(1, pLab);
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status = visitedCalc77_3.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(3, pLab);
				}
			}

			break;
		}
		case 4: {
			if (isFinishing) {
				break;
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease())
					if (calcRes.updateIdx(pLab->getPos()); true) {
						if (!genmc::isa<InitLabel>(pLab)) {
							worklist.emplace_back(0, pLab);
						}
					}
			if (auto *iLab = genmc::dyn_cast_if_present<InitLabel>(g.rf_pred(lab));
			    iLab) {
				calcRes.update(
					iLab->locView(genmc::cast<ReadLabel>(lab)->getAddr(), 1));
			}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease()) {
					if (!genmc::isa<InitLabel>(pLab)) {
						worklist.emplace_back(1, pLab);
					}
				}
			if (auto *iLab = genmc::dyn_cast_if_present<InitLabel>(g.rf_pred(lab));
			    iLab) {
				calcRes.update(
					iLab->locView(genmc::cast<ReadLabel>(lab)->getAddr(), 1));
			}
			if (auto pLab = g.rf_pred(lab); pLab) {
				if (!genmc::isa<InitLabel>(pLab)) {
					auto status =
						visitedCalc77_3.getStatus(pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					}
				}
			}
			if (auto *iLab = genmc::dyn_cast_if_present<InitLabel>(g.rf_pred(lab));
			    iLab) {
				calcRes.update(
					iLab->locView(genmc::cast<ReadLabel>(lab)->getAddr(), 1));
			}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					if (!genmc::isa<InitLabel>(pLab)) {
						auto status = visitedCalc77_5.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(5, pLab);
						}
					}
				}

			break;
		}
		case 5: {
			if (isFinishing) {
				visitedCalc77_5.setStatus(lab->getStamp().get(), NodeStatus::left);
				break;
			}

			auto status = visitedCalc77_5.getStatus(lab->getStamp().get());
			if (status != NodeStatus::unseen)
				break; /* already explored */

			worklist.emplace_back(5, lab, true);
			visitedCalc77_5.setStatus(lab->getStamp().get(), NodeStatus::entered);

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					worklist.emplace_back(4, pLab);
				}

			break;
		}
		case 6: {
			if (isFinishing) {
				visitedCalc77_6.setStatus(lab->getStamp().get(), NodeStatus::left);
				break;
			}

			auto status = visitedCalc77_6.getStatus(lab->getStamp().get());
			if (status != NodeStatus::unseen)
				break; /* already explored */

			worklist.emplace_back(6, lab, true);
			visitedCalc77_6.setStatus(lab->getStamp().get(), NodeStatus::entered);

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease())
					if (calcRes.updateIdx(pLab->getPos()); true) {
						if (!genmc::isa<InitLabel>(pLab)) {
							worklist.emplace_back(0, pLab);
						}
					}
			if (auto *iLab = genmc::dyn_cast_if_present<InitLabel>(g.rf_pred(lab));
			    iLab) {
				calcRes.update(
					iLab->locView(genmc::cast<ReadLabel>(lab)->getAddr(), 1));
			}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease()) {
					if (!genmc::isa<InitLabel>(pLab)) {
						worklist.emplace_back(1, pLab);
					}
				}
			if (auto *iLab = genmc::dyn_cast_if_present<InitLabel>(g.rf_pred(lab));
			    iLab) {
				calcRes.update(
					iLab->locView(genmc::cast<ReadLabel>(lab)->getAddr(), 1));
			}
			if (auto pLab = g.rf_pred(lab); pLab) {
				if (!genmc::isa<InitLabel>(pLab)) {
					auto status =
						visitedCalc77_3.getStatus(pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					}
				}
			}
			if (auto *iLab = genmc::dyn_cast_if_present<InitLabel>(g.rf_pred(lab));
			    iLab) {
				calcRes.update(
					iLab->locView(genmc::cast<ReadLabel>(lab)->getAddr(), 1));
			}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					if (!genmc::isa<InitLabel>(pLab)) {
						auto status = visitedCalc77_5.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(5, pLab);
						}
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status = visitedCalc77_6.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(6, pLab);
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

View IMMChecker::visitCalc77(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();
	View calcRes;

	calcRes.updateIdx(lab->getPos());
	visitedCalc77_3.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedCalc77_5.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedCalc77_6.maybeClearResize(g.getMaxStamp().get() + 1);

	/* Explore from all accepting states using DFS */
	std::vector<DFSWorklistEntry> startStates = {
		{2, lab},
	};

	visitCalc77Iterative(startStates, calcRes);
	return calcRes;
}

auto IMMChecker::checkCalc77(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	return visitCalc77(lab);
}

bool IMMChecker::visitCalc90Iterative(std::vector<DFSWorklistEntry> &worklist, View &calcRes) const
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
				if (calcRes.update(pLab->view(2)); true) {
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
					iLab->locView(genmc::cast<ReadLabel>(lab)->getAddr(), 2));
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

View IMMChecker::visitCalc90(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();
	View calcRes;

	calcRes.updateIdx(lab->getPos());

	/* Explore from all accepting states using DFS */
	std::vector<DFSWorklistEntry> startStates = {
		{2, lab},
	};

	visitCalc90Iterative(startStates, calcRes);
	return calcRes;
}

auto IMMChecker::checkCalc90(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	return visitCalc90(lab);
}

void IMMChecker::calculateSaved([[maybe_unused]] EventLabel *lab) {}

void IMMChecker::calculateViews(EventLabel *lab)
{
	lab->setViews({});

	{
		lab->addView({});
		lab->setView(std::move(checkCalc76(lab)), 0);
	}
	if (!getConf()->collectLinSpec && !getConf()->checkLinSpec)
		lab->addView({});
	else {
		lab->addView({});
		lab->setView(std::move(checkCalc77(lab)), 1);
	}

	{
		lab->addView({});
		lab->setView(std::move(checkCalc90(lab)), 2);
	}
}

void IMMChecker::updateMMViews(EventLabel *lab)
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

/* Whether END is reachable from BEGIN in ADDR's coherence order. It is not
 * when coherence contradicts hb, which a consistent graph never does, but an
 * event added before events kept further on in its thread can see before
 * the graph is checked again, typically because of a kept event the new path
 * skips (it goes once the thread passes its place): then every position is a
 * candidate, and GenMC's later checks drop the inconsistent ones */
static auto isCoRange(ExecutionGraph &g, SAddr addr, ExecutionGraph::co_iterator begin,
		      ExecutionGraph::co_iterator end) -> bool
{
	for (auto it = begin; it != end; ++it)
		if (it == std::ranges::end(g.co(addr)))
			return false;
	return true;
}

auto IMMChecker::getCoherentStores(ReadLabel *rLab) -> std::vector<EventLabel *>
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
	if (!isCoRange(g, rLab->getAddr(), begIt, endIt)) {
		stores.clear();
		stores.push_back(g.getInitLabel());
		begIt = std::ranges::begin(g.co(rLab->getAddr()));
		endIt = std::ranges::end(g.co(rLab->getAddr()));
	}
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

void IMMChecker::filterCoherentRevisits(WriteLabel *sLab, std::vector<ReadLabel *> &ls)
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

	/* ...and also exclude (mo^-1; rf?; (hb^-1)?; sb^-1)-after reads in the *resulting* graph.
	 * An RMW read's own write part, and the reads of it, are no such events:
	 * revisited, the read reads SLAB, and its write moves right after SLAB
	 * in coherence (a write part kept while its read was deleted and
	 * executed anew, see updateKeptRMWWrite()) */
	auto &before = sLab->getPrefixView();
	auto moInvOptRfs = getMOInvOptRfAfter(sLab);
	ls.erase(std::remove_if(
			 ls.begin(), ls.end(),
			 [&](auto &eLab) {
				 auto v = g.getViewFromStamp(eLab->getStamp());
				 v->update(before);
				 const auto *own = eLab->isRMW() ? g.po_imm_succ(eLab) : nullptr;
				 auto isOwn = [&](const EventLabel *evLab) {
					 const auto *rLab = genmc::dyn_cast<ReadLabel>(evLab);
					 return own &&
						(evLab == own || (rLab && rLab->getRf() == own));
				 };
				 return std::any_of(
					 moInvOptRfs.begin(), moInvOptRfs.end(), [&](auto &evLab) {
						 return !isOwn(evLab) &&
							v->contains(evLab->getPos()) && // stays in
											// graph?
							g.po_imm_pred(evLab)->view(0).contains(
								eLab->getPos()); // po-pred to check
										 // evLab != rLab
					 });
			 }),
		 ls.end());
}

auto IMMChecker::getCoherentPlacings(WriteLabel *wLab) -> std::vector<EventLabel *>
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
	if (!isCoRange(g, wLab->getAddr(), rangeBegin, rangeEnd)) {
		rangeBegin = std::ranges::begin(g.co(wLab->getAddr()));
		rangeEnd = std::ranges::end(g.co(wLab->getAddr()));
	}
	auto cos = std::ranges::subrange(rangeBegin, rangeEnd) |
		   std::views::filter([&](auto &sLab) { return !sLab.isRMW(); }) |
		   std::views::transform([&](auto &sLab) {
			   auto *pLab = g.co_imm_pred(&sLab);
			   return pLab ? (EventLabel *)pLab : (EventLabel *)g.getInitLabel();
		   });
	std::ranges::copy(cos, std::back_inserter(result));
	/* The range's end bounds it from above (dependency-tracking models: the
	 * first write a later access of the thread wrote or read). If it is an
	 * RMW's write, the placing right before it comes between that write and
	 * the write its read reads, as the ones the filter drops do */
	if (rangeEnd == std::ranges::end(g.co(wLab->getAddr())))
		result.push_back(g.co_max(wLab->getAddr()));
	else if (!rangeEnd->isRMW())
		result.push_back(!g.co_imm_pred(&*rangeEnd)
					 ? (EventLabel *)g.getInitLabel()
					 : (EventLabel *)g.co_imm_pred(&*rangeEnd));
	return result;
}
bool IMMChecker::visitCoherenceIterative(std::vector<DFSWorklistEntry> &worklist,
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
					auto status = visitedCoherence_8.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(8, pLab);
					}
				}
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedCoherence_10.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(10, pLab);
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
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedCoherence_4.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(4, pLab);
						}
					}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedCoherence_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedCoherence_6.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() &&
				    genmc::isa<ThreadJoinLabel>(pLab)) {
					auto status = visitedCoherence_6.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
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
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					worklist.emplace_back(0, pLab);
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<ThreadCreateLabel>(pLab)) {
					worklist.emplace_back(0, pLab);
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedCoherence_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<ThreadCreateLabel>(pLab)) {
					auto status = visitedCoherence_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status = visitedCoherence_3.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(3, pLab);
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedCoherence_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedCoherence_6.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
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
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease()) {
					worklist.emplace_back(0, pLab);
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease()) {
					auto status = visitedCoherence_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab) {
				auto status = visitedCoherence_3.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(3, pLab);
				}
			}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease()) {
					auto status = visitedCoherence_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					worklist.emplace_back(5, pLab);
				}

			break;
		}
		case 5: {
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
					auto status = visitedCoherence_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					}
				}

			break;
		}
		case 6: {
			if (isFinishing) {
				visitedCoherence_6.setStatus(lab->getStamp().get(),
							     NodeStatus::left);
				break;
			}

			auto status = visitedCoherence_6.getStatus(lab->getStamp().get());
			if (status != NodeStatus::unseen)
				break; /* already explored */

			worklist.emplace_back(6, lab, true);
			visitedCoherence_6.setStatus(lab->getStamp().get(), NodeStatus::entered);

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status = visitedCoherence_4.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(4, pLab);
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status = visitedCoherence_6.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(6, pLab);
				}
			}

			break;
		}
		case 7: {
			if (isFinishing) {
				visitedCoherence_7.setStatus(lab->getStamp().get(),
							     NodeStatus::left);
				break;
			}

			auto status = visitedCoherence_7.getStatus(lab->getStamp().get());
			if (status != NodeStatus::unseen)
				break; /* already explored */

			worklist.emplace_back(7, lab, true);
			visitedCoherence_7.setStatus(lab->getStamp().get(), NodeStatus::entered);

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
				if (auto *pLab = &tmp; true)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedCoherence_4.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(4, pLab);
						}
					}
			if (auto pLab = g.co_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedCoherence_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedCoherence_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					}
				}
			for (auto &tmp : g.fr_imm_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedCoherence_7.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(7, pLab);
					}
				}
			if (auto pLab = g.co_imm_pred(lab); pLab) {
				auto status = visitedCoherence_7.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(7, pLab);
				}
			}
			if (auto pLab = g.rf_pred(lab); pLab) {
				auto status = visitedCoherence_7.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(7, pLab);
				}
			}

			break;
		}
		case 8: {
			if (isFinishing) {
				visitedCoherence_8.setStatus(lab->getStamp().get(),
							     NodeStatus::left);
				break;
			}

			auto status = visitedCoherence_8.getStatus(lab->getStamp().get());
			if (status != NodeStatus::unseen)
				break; /* already explored */

			worklist.emplace_back(8, lab, true);
			visitedCoherence_8.setStatus(lab->getStamp().get(), NodeStatus::entered);

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedCoherence_7.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(7, pLab);
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status = visitedCoherence_7.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(7, pLab);
				}
			}
			if (auto pLab = g.tc_pred(lab); pLab) {
				auto status = visitedCoherence_7.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(7, pLab);
				}
			}
			if (auto pLab = g.tj_pred(lab); pLab) {
				auto status = visitedCoherence_7.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(7, pLab);
				}
			}
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedCoherence_8.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(8, pLab);
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status = visitedCoherence_8.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(8, pLab);
				}
			}
			if (auto pLab = g.tc_pred(lab); pLab) {
				auto status = visitedCoherence_8.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(8, pLab);
				}
			}
			if (auto pLab = g.tj_pred(lab); pLab) {
				auto status = visitedCoherence_8.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(8, pLab);
				}
			}
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedCoherence_10.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(10, pLab);
						}
					}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedCoherence_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedCoherence_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() &&
				    genmc::isa<ThreadJoinLabel>(pLab)) {
					auto status = visitedCoherence_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					}
				}

			break;
		}
		case 9: {
			if (isFinishing) {
				visitedCoherence_9.setStatus(lab->getStamp().get(),
							     NodeStatus::left);
				break;
			}

			auto status = visitedCoherence_9.getStatus(lab->getStamp().get());
			if (status != NodeStatus::unseen)
				break; /* already explored */

			worklist.emplace_back(9, lab, true);
			visitedCoherence_9.setStatus(lab->getStamp().get(), NodeStatus::entered);

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedCoherence_7.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(7, pLab);
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<ThreadCreateLabel>(pLab)) {
					auto status = visitedCoherence_7.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(7, pLab);
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedCoherence_8.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(8, pLab);
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<ThreadCreateLabel>(pLab)) {
					auto status = visitedCoherence_8.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(8, pLab);
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status = visitedCoherence_9.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(9, pLab);
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedCoherence_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedCoherence_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					}
				}

			break;
		}
		case 10: {
			if (isFinishing) {
				visitedCoherence_10.setStatus(lab->getStamp().get(),
							      NodeStatus::left);
				break;
			}

			auto status = visitedCoherence_10.getStatus(lab->getStamp().get());
			if (status != NodeStatus::unseen)
				break; /* already explored */

			worklist.emplace_back(10, lab, true);
			visitedCoherence_10.setStatus(lab->getStamp().get(), NodeStatus::entered);

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease()) {
					auto status = visitedCoherence_7.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(7, pLab);
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease()) {
					auto status = visitedCoherence_8.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(8, pLab);
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab) {
				auto status = visitedCoherence_9.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(9, pLab);
				}
			}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease()) {
					auto status = visitedCoherence_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					worklist.emplace_back(11, pLab);
				}

			break;
		}
		case 11: {
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
					auto status = visitedCoherence_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					}
				}

			break;
		}
		case 12: {
			if (isFinishing) {
				visitedCoherence_12.setStatus(lab->getStamp().get(),
							      NodeStatus::left);
				break;
			}

			auto status = visitedCoherence_12.getStatus(lab->getStamp().get());
			if (status != NodeStatus::unseen)
				break; /* already explored */

			worklist.emplace_back(12, lab, true);
			visitedCoherence_12.setStatus(lab->getStamp().get(), NodeStatus::entered);

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status = visitedCoherence_10.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(10, pLab);
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status = visitedCoherence_12.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(12, pLab);
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

bool IMMChecker::visitCoherenceRelinche(const ExecutionGraph &g) const
{
	for (auto &lab : g.labels()) {
		if (!genmc::isa<MethodBeginLabel>(&lab))
			continue;

		visitedCoherence_2.maybeClearResize(g.getMaxStamp().get() + 1);
		visitedCoherence_3.maybeClearResize(g.getMaxStamp().get() + 1);
		visitedCoherence_4.maybeClearResize(g.getMaxStamp().get() + 1);
		visitedCoherence_6.maybeClearResize(g.getMaxStamp().get() + 1);
		visitedCoherence_7.maybeClearResize(g.getMaxStamp().get() + 1);
		visitedCoherence_8.maybeClearResize(g.getMaxStamp().get() + 1);
		visitedCoherence_9.maybeClearResize(g.getMaxStamp().get() + 1);
		visitedCoherence_10.maybeClearResize(g.getMaxStamp().get() + 1);
		visitedCoherence_12.maybeClearResize(g.getMaxStamp().get() + 1);

		/* Explore from this accepting state using DFS */
		std::vector<DFSWorklistEntry> startState = {{1, &lab}};
		if (!visitCoherenceIterative(startState, &lab /* initLab */))
			return false;
	}
	return true;
}

bool IMMChecker::visitConsAcyclic1Iterative(std::vector<DFSWorklistEntry> &worklist) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		case 0: {
			if (isFinishing) {
				visitedConsAcyclic1Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic1_0.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1Search.enter(visitedConsAcyclic1_0,
								lab->getStamp().get(), false);
				worklist.emplace_back(0, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_0,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isSC()) {
					auto status = visitedConsAcyclic1_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_19,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 1: {
			if (isFinishing) {
				visitedConsAcyclic1Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic1_1.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1Search.enter(visitedConsAcyclic1_1,
								lab->getStamp().get(), false);
				worklist.emplace_back(1, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_1,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic1_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_0,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_0.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(0, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_0,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tc_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_0.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(0, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_0,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tj_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_0.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(0, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_0,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic1_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_1,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_1.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(1, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_1,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tc_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_1.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(1, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_1,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tj_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_1.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(1, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_1,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedConsAcyclic1_3.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(3, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_3,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedConsAcyclic1_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_3,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic1_5.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(5, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_5,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() &&
				    genmc::isa<ThreadJoinLabel>(pLab)) {
					auto status = visitedConsAcyclic1_5.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(5, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_5,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 2: {
			if (isFinishing) {
				visitedConsAcyclic1Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic1_2.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1Search.enter(visitedConsAcyclic1_2,
								lab->getStamp().get(), false);
				worklist.emplace_back(2, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_2,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic1_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_0,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<ThreadCreateLabel>(pLab)) {
					auto status = visitedConsAcyclic1_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_0,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic1_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_1,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<ThreadCreateLabel>(pLab)) {
					auto status = visitedConsAcyclic1_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_1,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_2.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(2, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_2,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic1_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_3,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic1_5.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(5, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_5,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 3: {
			if (isFinishing) {
				visitedConsAcyclic1Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic1_3.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1Search.enter(visitedConsAcyclic1_3,
								lab->getStamp().get(), false);
				worklist.emplace_back(3, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_3,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease()) {
					auto status = visitedConsAcyclic1_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_0,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease()) {
					auto status = visitedConsAcyclic1_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_1,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_2.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(2, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_2,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease()) {
					auto status = visitedConsAcyclic1_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_3,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					worklist.emplace_back(4, pLab);
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
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_3,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 5: {
			if (isFinishing) {
				visitedConsAcyclic1Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic1_5.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1Search.enter(visitedConsAcyclic1_5,
								lab->getStamp().get(), false);
				worklist.emplace_back(5, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_5,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_3.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(3, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_3,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_5.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(5, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_5,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 6: {
			if (isFinishing) {
				visitedConsAcyclic1Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic1_6.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1Search.enter(visitedConsAcyclic1_6,
								lab->getStamp().get(), false);
				worklist.emplace_back(6, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_6,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic1_6.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_6,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_6.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(6, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_6,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tc_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_6.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(6, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_6,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tj_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_6.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(6, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_6,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedConsAcyclic1_8.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(8, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_8,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedConsAcyclic1_8.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(8, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_8,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic1_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_10,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() &&
				    genmc::isa<ThreadJoinLabel>(pLab)) {
					auto status = visitedConsAcyclic1_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_10,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isSC() && genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic1_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_19,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 7: {
			if (isFinishing) {
				visitedConsAcyclic1Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic1_7.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1Search.enter(visitedConsAcyclic1_7,
								lab->getStamp().get(), false);
				worklist.emplace_back(7, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_7,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic1_6.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_6,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<ThreadCreateLabel>(pLab)) {
					auto status = visitedConsAcyclic1_6.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_6,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_7.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(7, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_7,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic1_8.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(8, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_8,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic1_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_10,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() && pLab->isSC() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic1_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_19,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 8: {
			if (isFinishing) {
				visitedConsAcyclic1Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic1_8.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1Search.enter(visitedConsAcyclic1_8,
								lab->getStamp().get(), false);
				worklist.emplace_back(8, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_8,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease()) {
					auto status = visitedConsAcyclic1_6.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_6,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_7.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(7, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_7,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease()) {
					auto status = visitedConsAcyclic1_8.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(8, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_8,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					worklist.emplace_back(9, pLab);
				}

			break;
		}
		case 9: {
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
					auto status = visitedConsAcyclic1_8.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(8, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_8,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 10: {
			if (isFinishing) {
				visitedConsAcyclic1Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic1_10.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1Search.enter(visitedConsAcyclic1_10,
								lab->getStamp().get(), false);
				worklist.emplace_back(10, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_10,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_8.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(8, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_8,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_10.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(10, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_10,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 11: {
			if (isFinishing) {
				visitedConsAcyclic1Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic1_11.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1Search.enter(visitedConsAcyclic1_11,
								lab->getStamp().get(), false);
				worklist.emplace_back(11, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_11,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic1_11.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(11, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_11,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_11.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(11, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_11,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tc_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_11.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(11, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_11,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tj_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_11.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(11, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_11,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic1_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_12,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.tc_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_12.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(12, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_12,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tj_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_12.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(12, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_12,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic1_13.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(13, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_13,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_13.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(13, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_13,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tc_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_13.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(13, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_13,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tj_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_13.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(13, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_13,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedConsAcyclic1_15.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(15, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_15,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedConsAcyclic1_15.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(15, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_15,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic1_17.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(17, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_17,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() &&
				    genmc::isa<ThreadJoinLabel>(pLab)) {
					auto status = visitedConsAcyclic1_17.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(17, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_17,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic1_18.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(18, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_18,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_18.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(18, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_18,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tc_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_18.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(18, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_18,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tj_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_18.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(18, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_18,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isSC() && genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic1_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_19,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isSC()) {
					auto status = visitedConsAcyclic1_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_19,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 12: {
			if (isFinishing) {
				visitedConsAcyclic1Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic1_12.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1Search.enter(visitedConsAcyclic1_12,
								lab->getStamp().get(), false);
				worklist.emplace_back(12, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_12,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_6.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(6, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_6,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedConsAcyclic1_8.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(8, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_8,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic1_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_10,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() &&
				    genmc::isa<ThreadJoinLabel>(pLab)) {
					auto status = visitedConsAcyclic1_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_10,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_12.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(12, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_12,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isSC()) {
					auto status = visitedConsAcyclic1_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_19,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 13: {
			if (isFinishing) {
				visitedConsAcyclic1Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic1_13.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1Search.enter(visitedConsAcyclic1_13,
								lab->getStamp().get(), false);
				worklist.emplace_back(13, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_13,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.fr_imm_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic1_6.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_6,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.co_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_6.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(6, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_6,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			for (auto &tmp : g.fr_imm_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedConsAcyclic1_8.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(8, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_8,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (auto pLab = g.co_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedConsAcyclic1_8.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(8, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_8,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.co_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_13.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(13, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_13,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			for (auto &tmp : g.fr_imm_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && pLab->isSC()) {
						auto status = visitedConsAcyclic1_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_19,
								   pLab->getStamp().get(), true)) {
							return false; /* cycle detected */
						}
					}
			if (auto pLab = g.co_imm_pred(lab); pLab)
				if (true && pLab->isSC()) {
					auto status = visitedConsAcyclic1_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_19,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 14: {
			if (isFinishing) {
				visitedConsAcyclic1Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic1_14.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1Search.enter(visitedConsAcyclic1_14,
								lab->getStamp().get(), false);
				worklist.emplace_back(14, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_14,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic1_11.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(11, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_11,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<ThreadCreateLabel>(pLab)) {
					auto status = visitedConsAcyclic1_11.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(11, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_11,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic1_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_12,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<ThreadCreateLabel>(pLab)) {
					auto status = visitedConsAcyclic1_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_12,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic1_13.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(13, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_13,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<ThreadCreateLabel>(pLab)) {
					auto status = visitedConsAcyclic1_13.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(13, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_13,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_14.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(14, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_14,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic1_15.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(15, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_15,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic1_17.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(17, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_17,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic1_18.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(18, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_18,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<ThreadCreateLabel>(pLab)) {
					auto status = visitedConsAcyclic1_18.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(18, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_18,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() && pLab->isSC() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic1_19.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(19, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_19,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 15: {
			if (isFinishing) {
				visitedConsAcyclic1Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic1_15.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1Search.enter(visitedConsAcyclic1_15,
								lab->getStamp().get(), false);
				worklist.emplace_back(15, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_15,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease()) {
					auto status = visitedConsAcyclic1_11.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(11, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_11,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease()) {
					auto status = visitedConsAcyclic1_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_12,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease()) {
					auto status = visitedConsAcyclic1_13.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(13, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_13,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_14.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(14, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_14,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease()) {
					auto status = visitedConsAcyclic1_15.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(15, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_15,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					worklist.emplace_back(16, pLab);
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease()) {
					auto status = visitedConsAcyclic1_18.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(18, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_18,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 16: {
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
					auto status = visitedConsAcyclic1_15.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(15, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_15,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 17: {
			if (isFinishing) {
				visitedConsAcyclic1Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic1_17.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1Search.enter(visitedConsAcyclic1_17,
								lab->getStamp().get(), false);
				worklist.emplace_back(17, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_17,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_15.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(15, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_15,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_17.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(17, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_17,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 18: {
			if (isFinishing) {
				visitedConsAcyclic1Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic1_18.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1Search.enter(visitedConsAcyclic1_18,
								lab->getStamp().get(), false);
				worklist.emplace_back(18, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_18,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.fr_imm_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic1_6.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_6,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.co_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_6.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(6, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_6,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.rf_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_6.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(6, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_6,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			for (auto &tmp : g.fr_imm_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedConsAcyclic1_8.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(8, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_8,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (auto pLab = g.co_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedConsAcyclic1_8.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(8, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_8,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedConsAcyclic1_8.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(8, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_8,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.fr_imm_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic1_18.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(18, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_18,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.co_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_18.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(18, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_18,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.rf_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic1_18.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(18, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_18,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 19: {
			if (isFinishing) {
				visitedConsAcyclic1Search.finish(true);
				break;
			}

			auto status = visitedConsAcyclic1_19.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic1Search.enter(visitedConsAcyclic1_19,
								lab->getStamp().get(), true);
				worklist.emplace_back(19, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic1Search.reach(visitedConsAcyclic1_19,
								   lab->getStamp().get(), true)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (true && lab->isSC())
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic1_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_1,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isSC())
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedConsAcyclic1_3.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(3, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_3,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && lab->isSC())
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && pLab->isAtLeastAcquire() &&
					    genmc::isa<FenceLabel>(pLab)) {
						auto status = visitedConsAcyclic1_5.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(5, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_5,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && lab->isSC())
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && pLab->isAtLeastAcquire() &&
					    genmc::isa<ThreadJoinLabel>(pLab)) {
						auto status = visitedConsAcyclic1_5.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(5, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_5,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && lab->isSC())
				for (auto &tmp : g.fr_imm_preds(lab))
					if (auto *pLab = &tmp; true) {
						auto status = visitedConsAcyclic1_6.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(6, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_6,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && lab->isSC())
				if (auto pLab = g.co_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic1_6.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_6,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isSC())
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic1_6.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_6,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isSC())
				if (auto pLab = g.rf_pred(lab); pLab) {
					auto status = visitedConsAcyclic1_6.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_6,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isSC())
				for (auto &tmp : g.fr_imm_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && pLab->isAtLeastAcquire()) {
							auto status =
								visitedConsAcyclic1_8.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(8, pLab);
							} else if (status == NodeStatus::entered &&
								   visitedConsAcyclic1Search.reach(
									   visitedConsAcyclic1_8,
									   pLab->getStamp().get(),
									   false)) {
								return false; /* cycle detected */
							}
						}
			if (true && lab->isSC())
				if (auto pLab = g.co_imm_pred(lab); pLab)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedConsAcyclic1_8.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(8, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_8,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && lab->isSC())
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedConsAcyclic1_8.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(8, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_8,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && lab->isSC())
				if (auto pLab = g.rf_pred(lab); pLab)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedConsAcyclic1_8.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(8, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_8,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && lab->isSC())
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && pLab->isAtLeastAcquire() &&
					    genmc::isa<FenceLabel>(pLab)) {
						auto status = visitedConsAcyclic1_10.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(10, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_10,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && lab->isSC())
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && pLab->isAtLeastAcquire() &&
					    genmc::isa<ThreadJoinLabel>(pLab)) {
						auto status = visitedConsAcyclic1_10.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(10, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_10,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && lab->isSC() && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic1_11.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(11, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_11,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isSC() && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic1_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_12,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isSC())
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic1_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_12,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isSC() && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic1_13.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(13, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_13,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isSC())
				if (auto pLab = g.co_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic1_13.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(13, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_13,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isAtLeastAcquire() && lab->isSC() &&
			    genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic1_15.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(15, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_15,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isSC() && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedConsAcyclic1_15.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(15, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_15,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && lab->isAtLeastAcquire() && lab->isSC() &&
			    genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic1_17.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(17, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_17,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isSC() && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && pLab->isAtLeastAcquire() &&
					    genmc::isa<FenceLabel>(pLab)) {
						auto status = visitedConsAcyclic1_17.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(17, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_17,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && lab->isSC() && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && pLab->isAtLeastAcquire() &&
					    genmc::isa<ThreadJoinLabel>(pLab)) {
						auto status = visitedConsAcyclic1_17.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(17, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_17,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && lab->isSC() && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic1_18.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(18, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic1Search.reach(
							   visitedConsAcyclic1_18,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isSC() && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && pLab->isSC() && genmc::isa<FenceLabel>(pLab)) {
						auto status = visitedConsAcyclic1_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_19,
								   pLab->getStamp().get(), true)) {
							return false; /* cycle detected */
						}
					}
			if (true && lab->isSC())
				for (auto &tmp : g.fr_imm_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && pLab->isSC()) {
							auto status =
								visitedConsAcyclic1_19.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(19, pLab);
							} else if (status == NodeStatus::entered &&
								   visitedConsAcyclic1Search.reach(
									   visitedConsAcyclic1_19,
									   pLab->getStamp().get(),
									   true)) {
								return false; /* cycle detected */
							}
						}
			if (true && lab->isSC())
				if (auto pLab = g.co_imm_pred(lab); pLab)
					if (true && pLab->isSC()) {
						auto status = visitedConsAcyclic1_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_19,
								   pLab->getStamp().get(), true)) {
							return false; /* cycle detected */
						}
					}
			if (true && lab->isSC())
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && pLab->isSC()) {
						auto status = visitedConsAcyclic1_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_19,
								   pLab->getStamp().get(), true)) {
							return false; /* cycle detected */
						}
					}
			if (true && lab->isSC())
				if (auto pLab = g.rf_pred(lab); pLab)
					if (true && pLab->isSC()) {
						auto status = visitedConsAcyclic1_19.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(19, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic1Search.reach(
								   visitedConsAcyclic1_19,
								   pLab->getStamp().get(), true)) {
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

bool IMMChecker::visitConsAcyclic1(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedConsAcyclic1Search.reset();
	visitedConsAcyclic1_0.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_1.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_2.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_3.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_5.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_6.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_7.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_8.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_10.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_11.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_12.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_13.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_14.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_15.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_17.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_18.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_19.maybeClearResize(g.getMaxStamp().get() + 1);

	/* States we need to explore from using DFS */
	std::vector<DFSWorklistEntry> startStates = {
		{19, lab}, {18, lab}, {15, lab}, {13, lab}, {12, lab}, {11, lab},
		{8, lab},  {6, lab},  {3, lab},	 {1, lab},  {0, lab},
	};

	return visitConsAcyclic1Iterative(startStates);
}

bool IMMChecker::visitConsAcyclic1Full(const ExecutionGraph &g) const
{
	visitedConsAcyclic1Search.reset();
	visitedConsAcyclic1_0.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_1.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_2.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_3.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_5.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_6.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_7.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_8.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_10.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_11.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_12.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_13.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_14.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_15.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_17.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_18.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic1_19.maybeClearResize(g.getMaxStamp().get() + 1);

	auto exploreLab = [&](auto &lab) {
		/* Explore from all accepting states using DFS */
		std::vector<DFSWorklistEntry> startStates = {
			{19, &lab},
		};

		return visitConsAcyclic1Iterative(startStates);
	};

	return std::ranges::all_of(g.labels(), exploreLab);
}

void IMMChecker::recomputeUnlessConsAcyclic1(const ExecutionGraph &g) const
{
	auto isMatchingLabel = [](const auto &l) {
		auto *lab = &l;
		if (true && lab->isSC())
			return true;
		return false;
	};

	cacheCounterUnlessConsAcyclic1 =
		std::count_if(g.label_begin(), g.label_end(), isMatchingLabel);
}

bool IMMChecker::visitUnlessConsAcyclic1(const EventLabel *lab) const
{
	auto &g = *lab->getParent();
	if (cacheCounterUnlessConsAcyclic1 == -1) /* Cache is empty */
		recomputeUnlessConsAcyclic1(g);
	return cacheCounterUnlessConsAcyclic1 < 2;
}

bool IMMChecker::checkConsAcyclic1(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	if (visitUnlessConsAcyclic1(lab))
		return true;

	return visitConsAcyclic1(lab);
}

bool IMMChecker::checkConsAcyclic1(const ExecutionGraph &g) const
{
	return visitConsAcyclic1Full(g);
}

bool IMMChecker::visitConsAcyclic2Iterative(std::vector<DFSWorklistEntry> &worklist) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		case 0: {
			if (isFinishing) {
				visitedConsAcyclic2Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic2_0.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2Search.enter(visitedConsAcyclic2_0,
								lab->getStamp().get(), false);
				worklist.emplace_back(0, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_0,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_0,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.ctrl_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_0,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_0,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfi_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_0.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(0, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_0,
									   pLab->getStamp().get(),
									   false)) {
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
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && pLab->isDependable()) {
						auto status = visitedConsAcyclic2_16.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(16, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic2Search.reach(
								   visitedConsAcyclic2_16,
								   pLab->getStamp().get(), true)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.ctrl_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && pLab->isDependable()) {
						auto status = visitedConsAcyclic2_16.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(16, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic2Search.reach(
								   visitedConsAcyclic2_16,
								   pLab->getStamp().get(), true)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && pLab->isDependable()) {
						auto status = visitedConsAcyclic2_16.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(16, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic2Search.reach(
								   visitedConsAcyclic2_16,
								   pLab->getStamp().get(), true)) {
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
					auto status = visitedConsAcyclic2_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_0,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW())) &&
				    pLab->isDependable()) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_16,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 2: {
			if (isFinishing) {
				visitedConsAcyclic2Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic2_2.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2Search.enter(visitedConsAcyclic2_2,
								lab->getStamp().get(), false);
				worklist.emplace_back(2, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_2,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_2.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(2, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_2,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_16.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(16, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_16,
									   pLab->getStamp().get(),
									   true)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 3: {
			if (isFinishing) {
				visitedConsAcyclic2Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic2_3.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2Search.enter(visitedConsAcyclic2_3,
								lab->getStamp().get(), false);
				worklist.emplace_back(3, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_3,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_3.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(3, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_3,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_16,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ThreadJoinLabel>(pLab)) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_16,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_16,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 4: {
			if (isFinishing) {
				visitedConsAcyclic2Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic2_4.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2Search.enter(visitedConsAcyclic2_4,
								lab->getStamp().get(), false);
				worklist.emplace_back(4, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_4,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.poloc_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_4.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(4, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_4,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.poloc_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_16,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 5: {
			if (isFinishing) {
				visitedConsAcyclic2Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic2_5.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2Search.enter(visitedConsAcyclic2_5,
								lab->getStamp().get(), false);
				worklist.emplace_back(5, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_5,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic2_5.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(5, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_5,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_5.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(5, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_5,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tc_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_5.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(5, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_5,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tj_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_5.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(5, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_5,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedConsAcyclic2_7.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(7, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic2Search.reach(
								   visitedConsAcyclic2_7,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedConsAcyclic2_7.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(7, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_7,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic2_9.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(9, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_9,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() &&
				    genmc::isa<ThreadJoinLabel>(pLab)) {
					auto status = visitedConsAcyclic2_9.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(9, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_9,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isSC() && genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_16,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 6: {
			if (isFinishing) {
				visitedConsAcyclic2Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic2_6.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2Search.enter(visitedConsAcyclic2_6,
								lab->getStamp().get(), false);
				worklist.emplace_back(6, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_6,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic2_5.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(5, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_5,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<ThreadCreateLabel>(pLab)) {
					auto status = visitedConsAcyclic2_5.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(5, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_5,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_6.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(6, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_6,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic2_7.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(7, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_7,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic2_9.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(9, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_9,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() && pLab->isSC() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_16,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 7: {
			if (isFinishing) {
				visitedConsAcyclic2Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic2_7.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2Search.enter(visitedConsAcyclic2_7,
								lab->getStamp().get(), false);
				worklist.emplace_back(7, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_7,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease()) {
					auto status = visitedConsAcyclic2_5.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(5, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_5,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_6.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(6, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_6,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease()) {
					auto status = visitedConsAcyclic2_7.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(7, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_7,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					worklist.emplace_back(8, pLab);
				}

			break;
		}
		case 8: {
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
					auto status = visitedConsAcyclic2_7.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(7, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_7,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 9: {
			if (isFinishing) {
				visitedConsAcyclic2Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic2_9.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2Search.enter(visitedConsAcyclic2_9,
								lab->getStamp().get(), false);
				worklist.emplace_back(9, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_9,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_7.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(7, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_7,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_9.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(9, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_9,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 10: {
			if (isFinishing) {
				visitedConsAcyclic2Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic2_10.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2Search.enter(visitedConsAcyclic2_10,
								lab->getStamp().get(), false);
				worklist.emplace_back(10, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_10,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.fr_imm_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic2_5.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(5, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_5,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.co_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_5.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(5, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_5,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.rf_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_5.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(5, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_5,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			for (auto &tmp : g.fr_imm_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedConsAcyclic2_7.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(7, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic2Search.reach(
								   visitedConsAcyclic2_7,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (auto pLab = g.co_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedConsAcyclic2_7.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(7, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_7,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedConsAcyclic2_7.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(7, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_7,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.fr_imm_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_10,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.co_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_10.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(10, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_10,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.rf_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_10.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(10, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_10,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 11: {
			if (isFinishing) {
				visitedConsAcyclic2Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic2_11.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2Search.enter(visitedConsAcyclic2_11,
								lab->getStamp().get(), false);
				worklist.emplace_back(11, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_11,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_10,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_10.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(10, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_10,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tc_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_10.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(10, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_10,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tj_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_10.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(10, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_10,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic2_11.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(11, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_11,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_11.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(11, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_11,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tc_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_11.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(11, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_11,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tj_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_11.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(11, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_11,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedConsAcyclic2_13.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(13, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic2Search.reach(
								   visitedConsAcyclic2_13,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedConsAcyclic2_13.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(13, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_13,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic2_15.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(15, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_15,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() &&
				    genmc::isa<ThreadJoinLabel>(pLab)) {
					auto status = visitedConsAcyclic2_15.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(15, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_15,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 12: {
			if (isFinishing) {
				visitedConsAcyclic2Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic2_12.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2Search.enter(visitedConsAcyclic2_12,
								lab->getStamp().get(), false);
				worklist.emplace_back(12, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_12,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_10,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<ThreadCreateLabel>(pLab)) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_10,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic2_11.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(11, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_11,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<ThreadCreateLabel>(pLab)) {
					auto status = visitedConsAcyclic2_11.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(11, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_11,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_12.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(12, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_12,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic2_13.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(13, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_13,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic2_15.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(15, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_15,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 13: {
			if (isFinishing) {
				visitedConsAcyclic2Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic2_13.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2Search.enter(visitedConsAcyclic2_13,
								lab->getStamp().get(), false);
				worklist.emplace_back(13, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_13,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease()) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_10,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease()) {
					auto status = visitedConsAcyclic2_11.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(11, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_11,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_12.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(12, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_12,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease()) {
					auto status = visitedConsAcyclic2_13.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(13, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_13,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					worklist.emplace_back(14, pLab);
				}

			break;
		}
		case 14: {
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
					auto status = visitedConsAcyclic2_13.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(13, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_13,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 15: {
			if (isFinishing) {
				visitedConsAcyclic2Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic2_15.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2Search.enter(visitedConsAcyclic2_15,
								lab->getStamp().get(), false);
				worklist.emplace_back(15, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_15,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_13.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(13, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_13,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_15.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(15, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_15,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 16: {
			if (isFinishing) {
				visitedConsAcyclic2Search.finish(true);
				break;
			}

			auto status = visitedConsAcyclic2_16.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic2Search.enter(visitedConsAcyclic2_16,
								lab->getStamp().get(), true);
				worklist.emplace_back(16, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_16,
								   lab->getStamp().get(), true)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (true && genmc::isa<WriteLabel>(lab) &&
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
						auto status = visitedConsAcyclic2_0.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(0, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic2Search.reach(
								   visitedConsAcyclic2_0,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true) {
						auto status = visitedConsAcyclic2_0.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(0, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic2Search.reach(
								   visitedConsAcyclic2_0,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.ctrl_preds(lab))
					if (auto *pLab = &tmp; true) {
						auto status = visitedConsAcyclic2_0.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(0, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic2Search.reach(
								   visitedConsAcyclic2_0,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.data_preds(lab))
					if (auto *pLab = &tmp; true) {
						auto status = visitedConsAcyclic2_0.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(0, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic2Search.reach(
								   visitedConsAcyclic2_0,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic2_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_2,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && genmc::isa<ThreadJoinLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic2_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_2,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isAtLeastRelease())
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic2_2.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(2, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_2,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_3.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(3, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_3,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (true && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.poloc_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic2_4.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(4, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_4,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isSC() && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic2_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_10,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isSC() && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic2_11.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(11, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_11,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isAtLeastAcquire() && lab->isSC() &&
			    genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic2_13.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(13, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_13,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isSC() && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedConsAcyclic2_13.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(13, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic2Search.reach(
								   visitedConsAcyclic2_13,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && lab->isAtLeastAcquire() && lab->isSC() &&
			    genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic2_15.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(15, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_15,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isSC() && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && pLab->isAtLeastAcquire() &&
					    genmc::isa<FenceLabel>(pLab)) {
						auto status = visitedConsAcyclic2_15.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(15, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic2Search.reach(
								   visitedConsAcyclic2_15,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && lab->isSC() && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && pLab->isAtLeastAcquire() &&
					    genmc::isa<ThreadJoinLabel>(pLab)) {
						auto status = visitedConsAcyclic2_15.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(15, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic2Search.reach(
								   visitedConsAcyclic2_15,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.detour_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_16,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_16,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ThreadJoinLabel>(pLab)) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_16,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_16,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfe_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic2_16.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(16, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic2Search.reach(visitedConsAcyclic2_16,
									   pLab->getStamp().get(),
									   true)) {
					return false; /* cycle detected */
				}
			}
			if (true && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_16,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}
			if (true && genmc::isa<ThreadJoinLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_16,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}
			if (true && genmc::isa<WriteLabel>(lab) &&
			    ((genmc::isa<ReadLabel>(lab) &&
			      genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
			     (genmc::isa<WriteLabel>(lab) &&
			      genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW())) &&
					    pLab->isDependable()) {
						auto status = visitedConsAcyclic2_16.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(16, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic2Search.reach(
								   visitedConsAcyclic2_16,
								   pLab->getStamp().get(), true)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && pLab->isDependable()) {
							auto status =
								visitedConsAcyclic2_16.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(16, pLab);
							} else if (status == NodeStatus::entered &&
								   visitedConsAcyclic2Search.reach(
									   visitedConsAcyclic2_16,
									   pLab->getStamp().get(),
									   true)) {
								return false; /* cycle detected */
							}
						}
			if (true && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.ctrl_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && pLab->isDependable()) {
							auto status =
								visitedConsAcyclic2_16.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(16, pLab);
							} else if (status == NodeStatus::entered &&
								   visitedConsAcyclic2Search.reach(
									   visitedConsAcyclic2_16,
									   pLab->getStamp().get(),
									   true)) {
								return false; /* cycle detected */
							}
						}
			if (true && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.data_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && pLab->isDependable()) {
							auto status =
								visitedConsAcyclic2_16.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(16, pLab);
							} else if (status == NodeStatus::entered &&
								   visitedConsAcyclic2Search.reach(
									   visitedConsAcyclic2_16,
									   pLab->getStamp().get(),
									   true)) {
								return false; /* cycle detected */
							}
						}
			if (true && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.poloc_imm_pred(lab); pLab)
					if (true && pLab->isAtLeastRelease() &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic2_16.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(16, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic2Search.reach(
								   visitedConsAcyclic2_16,
								   pLab->getStamp().get(), true)) {
							return false; /* cycle detected */
						}
					}
			if (true && lab->isAtLeastRelease())
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic2_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic2Search.reach(
							   visitedConsAcyclic2_16,
							   pLab->getStamp().get(), true)) {
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

bool IMMChecker::visitConsAcyclic2(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedConsAcyclic2Search.reset();
	visitedConsAcyclic2_0.maybeClearResize(g.getMaxStamp().get() + 1);
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
	visitedConsAcyclic2_15.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_16.maybeClearResize(g.getMaxStamp().get() + 1);

	/* States we need to explore from using DFS */
	std::vector<DFSWorklistEntry> startStates = {
		{13, lab}, {11, lab}, {10, lab}, {7, lab}, {5, lab},
	};

	return visitConsAcyclic2Iterative(startStates);
}

bool IMMChecker::visitConsAcyclic2Full(const ExecutionGraph &g) const
{
	visitedConsAcyclic2Search.reset();
	visitedConsAcyclic2_0.maybeClearResize(g.getMaxStamp().get() + 1);
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
	visitedConsAcyclic2_15.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic2_16.maybeClearResize(g.getMaxStamp().get() + 1);

	auto exploreLab = [&](auto &lab) {
		/* Explore from all accepting states using DFS */
		std::vector<DFSWorklistEntry> startStates = {
			{16, &lab},
		};

		return visitConsAcyclic2Iterative(startStates);
	};

	return std::ranges::all_of(g.labels(), exploreLab);
}

bool IMMChecker::checkConsAcyclic2(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	return visitConsAcyclic2(lab);
}

bool IMMChecker::checkConsAcyclic2(const ExecutionGraph &g) const
{
	return visitConsAcyclic2Full(g);
}

bool IMMChecker::visitConsAcyclic3Iterative(std::vector<DFSWorklistEntry> &worklist) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		case 0: {
			if (isFinishing) {
				visitedConsAcyclic3Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic3_0.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic3Search.enter(visitedConsAcyclic3_0,
								lab->getStamp().get(), false);
				worklist.emplace_back(0, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_0,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.ctrl_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic3_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_0,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic3_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_0,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfi_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_0.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(0, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_0,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			for (auto &tmp : g.ctrl_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic3_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_1,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic3_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_1,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfi_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_1.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(1, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_1,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.rfi_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					worklist.emplace_back(2, pLab);
				}
			for (auto &tmp : g.ctrl_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && pLab->isDependable()) {
						auto status = visitedConsAcyclic3_17.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(17, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic3Search.reach(
								   visitedConsAcyclic3_17,
								   pLab->getStamp().get(), true)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.data_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && pLab->isDependable()) {
						auto status = visitedConsAcyclic3_17.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(17, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic3Search.reach(
								   visitedConsAcyclic3_17,
								   pLab->getStamp().get(), true)) {
							return false; /* cycle detected */
						}
					}

			break;
		}
		case 1: {
			if (isFinishing) {
				visitedConsAcyclic3Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic3_1.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic3Search.enter(visitedConsAcyclic3_1,
								lab->getStamp().get(), false);
				worklist.emplace_back(1, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_1,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic3_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_0,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic3_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_1,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_1.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(1, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_1,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			for (auto &tmp : g.addr_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && pLab->isDependable()) {
						auto status = visitedConsAcyclic3_17.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(17, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic3Search.reach(
								   visitedConsAcyclic3_17,
								   pLab->getStamp().get(), true)) {
							return false; /* cycle detected */
						}
					}

			break;
		}
		case 2: {
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
					auto status = visitedConsAcyclic3_0.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(0, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_0,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					auto status = visitedConsAcyclic3_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_1,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ReadLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW())) &&
				    pLab->isDependable()) {
					auto status = visitedConsAcyclic3_17.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(17, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_17,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 3: {
			if (isFinishing) {
				visitedConsAcyclic3Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic3_3.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic3Search.enter(visitedConsAcyclic3_3,
								lab->getStamp().get(), false);
				worklist.emplace_back(3, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_3,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_3.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(3, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_3,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_17.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(17, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_17,
									   pLab->getStamp().get(),
									   true)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 4: {
			if (isFinishing) {
				visitedConsAcyclic3Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic3_4.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic3Search.enter(visitedConsAcyclic3_4,
								lab->getStamp().get(), false);
				worklist.emplace_back(4, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_4,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_4.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(4, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_4,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic3_17.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(17, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_17,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ThreadJoinLabel>(pLab)) {
					auto status = visitedConsAcyclic3_17.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(17, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_17,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedConsAcyclic3_17.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(17, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_17,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 5: {
			if (isFinishing) {
				visitedConsAcyclic3Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic3_5.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic3Search.enter(visitedConsAcyclic3_5,
								lab->getStamp().get(), false);
				worklist.emplace_back(5, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_5,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.poloc_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_5.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(5, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_5,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.poloc_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<WriteLabel>(pLab)) {
					auto status = visitedConsAcyclic3_17.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(17, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_17,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 6: {
			if (isFinishing) {
				visitedConsAcyclic3Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic3_6.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic3Search.enter(visitedConsAcyclic3_6,
								lab->getStamp().get(), false);
				worklist.emplace_back(6, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_6,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic3_6.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_6,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_6.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(6, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_6,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tc_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_6.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(6, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_6,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tj_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_6.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(6, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_6,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedConsAcyclic3_8.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(8, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic3Search.reach(
								   visitedConsAcyclic3_8,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedConsAcyclic3_8.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(8, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_8,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic3_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_10,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() &&
				    genmc::isa<ThreadJoinLabel>(pLab)) {
					auto status = visitedConsAcyclic3_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_10,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isSC() && genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic3_17.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(17, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_17,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 7: {
			if (isFinishing) {
				visitedConsAcyclic3Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic3_7.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic3Search.enter(visitedConsAcyclic3_7,
								lab->getStamp().get(), false);
				worklist.emplace_back(7, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_7,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic3_6.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_6,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<ThreadCreateLabel>(pLab)) {
					auto status = visitedConsAcyclic3_6.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_6,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_7.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(7, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_7,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic3_8.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(8, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_8,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic3_10.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(10, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_10,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() && pLab->isSC() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic3_17.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(17, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_17,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 8: {
			if (isFinishing) {
				visitedConsAcyclic3Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic3_8.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic3Search.enter(visitedConsAcyclic3_8,
								lab->getStamp().get(), false);
				worklist.emplace_back(8, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_8,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease()) {
					auto status = visitedConsAcyclic3_6.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_6,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_7.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(7, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_7,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease()) {
					auto status = visitedConsAcyclic3_8.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(8, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_8,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					worklist.emplace_back(9, pLab);
				}

			break;
		}
		case 9: {
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
					auto status = visitedConsAcyclic3_8.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(8, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_8,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 10: {
			if (isFinishing) {
				visitedConsAcyclic3Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic3_10.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic3Search.enter(visitedConsAcyclic3_10,
								lab->getStamp().get(), false);
				worklist.emplace_back(10, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_10,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_8.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(8, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_8,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_10.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(10, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_10,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 11: {
			if (isFinishing) {
				visitedConsAcyclic3Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic3_11.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic3Search.enter(visitedConsAcyclic3_11,
								lab->getStamp().get(), false);
				worklist.emplace_back(11, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_11,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.fr_imm_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic3_6.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(6, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_6,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.co_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_6.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(6, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_6,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.rf_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_6.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(6, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_6,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			for (auto &tmp : g.fr_imm_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedConsAcyclic3_8.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(8, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic3Search.reach(
								   visitedConsAcyclic3_8,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (auto pLab = g.co_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedConsAcyclic3_8.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(8, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_8,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedConsAcyclic3_8.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(8, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_8,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			for (auto &tmp : g.fr_imm_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic3_11.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(11, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_11,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.co_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_11.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(11, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_11,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.rf_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_11.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(11, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_11,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 12: {
			if (isFinishing) {
				visitedConsAcyclic3Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic3_12.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic3Search.enter(visitedConsAcyclic3_12,
								lab->getStamp().get(), false);
				worklist.emplace_back(12, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_12,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic3_11.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(11, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_11,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_11.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(11, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_11,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tc_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_11.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(11, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_11,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tj_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_11.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(11, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_11,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic3_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_12,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_12.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(12, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_12,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tc_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_12.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(12, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_12,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.tj_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_12.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(12, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_12,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			for (auto &tmp : g.lin_preds(lab))
				if (auto *pLab = &tmp; true)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedConsAcyclic3_14.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(14, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic3Search.reach(
								   visitedConsAcyclic3_14,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedConsAcyclic3_14.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(14, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_14,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic3_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_16,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() &&
				    genmc::isa<ThreadJoinLabel>(pLab)) {
					auto status = visitedConsAcyclic3_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_16,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 13: {
			if (isFinishing) {
				visitedConsAcyclic3Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic3_13.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic3Search.enter(visitedConsAcyclic3_13,
								lab->getStamp().get(), false);
				worklist.emplace_back(13, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_13,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic3_11.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(11, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_11,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<ThreadCreateLabel>(pLab)) {
					auto status = visitedConsAcyclic3_11.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(11, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_11,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic3_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_12,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease() &&
				    genmc::isa<ThreadCreateLabel>(pLab)) {
					auto status = visitedConsAcyclic3_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_12,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_13.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(13, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_13,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic3_14.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(14, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_14,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease() &&
				    genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic3_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_16,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 14: {
			if (isFinishing) {
				visitedConsAcyclic3Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic3_14.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic3Search.enter(visitedConsAcyclic3_14,
								lab->getStamp().get(), false);
				worklist.emplace_back(14, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_14,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease()) {
					auto status = visitedConsAcyclic3_11.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(11, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_11,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastRelease()) {
					auto status = visitedConsAcyclic3_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_12,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_13.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(13, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_13,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire() && pLab->isAtLeastRelease()) {
					auto status = visitedConsAcyclic3_14.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(14, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_14,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rf_pred(lab); pLab)
				if (true && genmc::isa<WriteLabel>(pLab) &&
				    ((genmc::isa<ReadLabel>(pLab) &&
				      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
				     (genmc::isa<WriteLabel>(pLab) &&
				      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
					worklist.emplace_back(15, pLab);
				}

			break;
		}
		case 15: {
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
					auto status = visitedConsAcyclic3_14.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(14, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_14,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}

			break;
		}
		case 16: {
			if (isFinishing) {
				visitedConsAcyclic3Search.finish(false);
				break;
			}

			auto status = visitedConsAcyclic3_16.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic3Search.enter(visitedConsAcyclic3_16,
								lab->getStamp().get(), false);
				worklist.emplace_back(16, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_16,
								   lab->getStamp().get(), false)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_14.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(14, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_14,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_16.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(16, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_16,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}

			break;
		}
		case 17: {
			if (isFinishing) {
				visitedConsAcyclic3Search.finish(true);
				break;
			}

			auto status = visitedConsAcyclic3_17.getStatus(lab->getStamp().get());
			if (status == NodeStatus::unseen) {
				visitedConsAcyclic3Search.enter(visitedConsAcyclic3_17,
								lab->getStamp().get(), true);
				worklist.emplace_back(17, lab, true /* isFinishing */);
			} else if (status == NodeStatus::entered &&
				   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_17,
								   lab->getStamp().get(), true)) {
				return false; /* cycle detected */
			} else {
				break; /* already explored */
			}

			[[maybe_unused]] auto &g = *lab->getParent();
			if (true && genmc::isa<WriteLabel>(lab) &&
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
						auto status = visitedConsAcyclic3_0.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(0, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic3Search.reach(
								   visitedConsAcyclic3_0,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true) {
						auto status = visitedConsAcyclic3_0.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(0, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic3Search.reach(
								   visitedConsAcyclic3_0,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.ctrl_preds(lab))
					if (auto *pLab = &tmp; true) {
						auto status = visitedConsAcyclic3_0.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(0, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic3Search.reach(
								   visitedConsAcyclic3_0,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.data_preds(lab))
					if (auto *pLab = &tmp; true) {
						auto status = visitedConsAcyclic3_0.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(0, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic3Search.reach(
								   visitedConsAcyclic3_0,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab) &&
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
						auto status = visitedConsAcyclic3_1.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(1, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic3Search.reach(
								   visitedConsAcyclic3_1,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true) {
						auto status = visitedConsAcyclic3_1.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(1, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic3Search.reach(
								   visitedConsAcyclic3_1,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.ctrl_preds(lab))
					if (auto *pLab = &tmp; true) {
						auto status = visitedConsAcyclic3_1.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(1, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic3Search.reach(
								   visitedConsAcyclic3_1,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.data_preds(lab))
					if (auto *pLab = &tmp; true) {
						auto status = visitedConsAcyclic3_1.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(1, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic3Search.reach(
								   visitedConsAcyclic3_1,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic3_1.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(1, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_1,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic3_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_3,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && genmc::isa<ThreadJoinLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic3_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_3,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isAtLeastRelease())
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic3_3.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(3, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_3,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_4.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(4, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_4,
									   pLab->getStamp().get(),
									   false)) {
					return false; /* cycle detected */
				}
			}
			if (true && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.poloc_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic3_5.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(5, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_5,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isSC() && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic3_11.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(11, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_11,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isSC() && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic3_12.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(12, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_12,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isAtLeastAcquire() && lab->isSC() &&
			    genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic3_14.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(14, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_14,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isSC() && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && pLab->isAtLeastAcquire()) {
						auto status = visitedConsAcyclic3_14.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(14, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic3Search.reach(
								   visitedConsAcyclic3_14,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && lab->isAtLeastAcquire() && lab->isSC() &&
			    genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic3_16.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(16, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_16,
							   pLab->getStamp().get(), false)) {
						return false; /* cycle detected */
					}
				}
			if (true && lab->isSC() && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && pLab->isAtLeastAcquire() &&
					    genmc::isa<FenceLabel>(pLab)) {
						auto status = visitedConsAcyclic3_16.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(16, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic3Search.reach(
								   visitedConsAcyclic3_16,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			if (true && lab->isSC() && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && pLab->isAtLeastAcquire() &&
					    genmc::isa<ThreadJoinLabel>(pLab)) {
						auto status = visitedConsAcyclic3_16.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(16, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic3Search.reach(
								   visitedConsAcyclic3_16,
								   pLab->getStamp().get(), false)) {
							return false; /* cycle detected */
						}
					}
			for (auto &tmp : g.detour_preds(lab))
				if (auto *pLab = &tmp; true) {
					auto status = visitedConsAcyclic3_17.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(17, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_17,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<FenceLabel>(pLab)) {
					auto status = visitedConsAcyclic3_17.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(17, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_17,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && genmc::isa<ThreadJoinLabel>(pLab)) {
					auto status = visitedConsAcyclic3_17.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(17, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_17,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.po_imm_pred(lab); pLab)
				if (true && pLab->isAtLeastAcquire()) {
					auto status = visitedConsAcyclic3_17.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(17, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_17,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}
			if (auto pLab = g.rfe_pred(lab); pLab) {
				auto status =
					visitedConsAcyclic3_17.getStatus(pLab->getStamp().get());
				if (status == NodeStatus::unseen) {
					worklist.emplace_back(17, pLab);
				} else if (status == NodeStatus::entered &&
					   visitedConsAcyclic3Search.reach(visitedConsAcyclic3_17,
									   pLab->getStamp().get(),
									   true)) {
					return false; /* cycle detected */
				}
			}
			if (true && genmc::isa<FenceLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic3_17.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(17, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_17,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}
			if (true && genmc::isa<ThreadJoinLabel>(lab))
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic3_17.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(17, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_17,
							   pLab->getStamp().get(), true)) {
						return false; /* cycle detected */
					}
				}
			if (true && genmc::isa<WriteLabel>(lab) &&
			    ((genmc::isa<ReadLabel>(lab) &&
			      genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
			     (genmc::isa<WriteLabel>(lab) &&
			      genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
				if (auto pLab = g.po_imm_pred(lab); pLab)
					if (true && genmc::isa<ReadLabel>(pLab) &&
					    ((genmc::isa<ReadLabel>(pLab) &&
					      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
					     (genmc::isa<WriteLabel>(pLab) &&
					      genmc::dyn_cast<WriteLabel>(pLab)->isRMW())) &&
					    pLab->isDependable()) {
						auto status = visitedConsAcyclic3_17.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(17, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic3Search.reach(
								   visitedConsAcyclic3_17,
								   pLab->getStamp().get(), true)) {
							return false; /* cycle detected */
						}
					}
			if (true && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.addr_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && pLab->isDependable()) {
							auto status =
								visitedConsAcyclic3_17.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(17, pLab);
							} else if (status == NodeStatus::entered &&
								   visitedConsAcyclic3Search.reach(
									   visitedConsAcyclic3_17,
									   pLab->getStamp().get(),
									   true)) {
								return false; /* cycle detected */
							}
						}
			if (true && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.ctrl_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && pLab->isDependable()) {
							auto status =
								visitedConsAcyclic3_17.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(17, pLab);
							} else if (status == NodeStatus::entered &&
								   visitedConsAcyclic3Search.reach(
									   visitedConsAcyclic3_17,
									   pLab->getStamp().get(),
									   true)) {
								return false; /* cycle detected */
							}
						}
			if (true && genmc::isa<WriteLabel>(lab))
				for (auto &tmp : g.data_preds(lab))
					if (auto *pLab = &tmp; true)
						if (true && pLab->isDependable()) {
							auto status =
								visitedConsAcyclic3_17.getStatus(
									pLab->getStamp().get());
							if (status == NodeStatus::unseen) {
								worklist.emplace_back(17, pLab);
							} else if (status == NodeStatus::entered &&
								   visitedConsAcyclic3Search.reach(
									   visitedConsAcyclic3_17,
									   pLab->getStamp().get(),
									   true)) {
								return false; /* cycle detected */
							}
						}
			if (true && genmc::isa<WriteLabel>(lab))
				if (auto pLab = g.poloc_imm_pred(lab); pLab)
					if (true && pLab->isAtLeastRelease() &&
					    genmc::isa<WriteLabel>(pLab)) {
						auto status = visitedConsAcyclic3_17.getStatus(
							pLab->getStamp().get());
						if (status == NodeStatus::unseen) {
							worklist.emplace_back(17, pLab);
						} else if (status == NodeStatus::entered &&
							   visitedConsAcyclic3Search.reach(
								   visitedConsAcyclic3_17,
								   pLab->getStamp().get(), true)) {
							return false; /* cycle detected */
						}
					}
			if (true && lab->isAtLeastRelease())
				if (auto pLab = g.po_imm_pred(lab); pLab) {
					auto status = visitedConsAcyclic3_17.getStatus(
						pLab->getStamp().get());
					if (status == NodeStatus::unseen) {
						worklist.emplace_back(17, pLab);
					} else if (status == NodeStatus::entered &&
						   visitedConsAcyclic3Search.reach(
							   visitedConsAcyclic3_17,
							   pLab->getStamp().get(), true)) {
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

bool IMMChecker::visitConsAcyclic3(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedConsAcyclic3Search.reset();
	visitedConsAcyclic3_0.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_1.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_3.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_4.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_5.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_6.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_7.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_8.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_10.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_11.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_12.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_13.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_14.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_16.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_17.maybeClearResize(g.getMaxStamp().get() + 1);

	/* States we need to explore from using DFS */
	std::vector<DFSWorklistEntry> startStates = {
		{14, lab}, {12, lab}, {11, lab}, {8, lab}, {6, lab},
	};

	return visitConsAcyclic3Iterative(startStates);
}

bool IMMChecker::visitConsAcyclic3Full(const ExecutionGraph &g) const
{
	visitedConsAcyclic3Search.reset();
	visitedConsAcyclic3_0.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_1.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_3.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_4.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_5.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_6.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_7.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_8.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_10.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_11.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_12.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_13.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_14.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_16.maybeClearResize(g.getMaxStamp().get() + 1);
	visitedConsAcyclic3_17.maybeClearResize(g.getMaxStamp().get() + 1);

	auto exploreLab = [&](auto &lab) {
		/* Explore from all accepting states using DFS */
		std::vector<DFSWorklistEntry> startStates = {
			{17, &lab},
		};

		return visitConsAcyclic3Iterative(startStates);
	};

	return std::ranges::all_of(g.labels(), exploreLab);
}

bool IMMChecker::checkConsAcyclic3(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	return visitConsAcyclic3(lab);
}

bool IMMChecker::checkConsAcyclic3(const ExecutionGraph &g) const
{
	return visitConsAcyclic3Full(g);
}

bool IMMChecker::visitError4([[maybe_unused]] const EventLabel *lab) const { return false; }

bool IMMChecker::visitLHSUnlessError4Iterative(std::vector<DFSWorklistEntry> &worklist,
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

bool IMMChecker::visitUnlessError4(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	auto &v = lab->view(0);

	/* Explore from all accepting states in LHS using DFS */
	std::vector<DFSWorklistEntry> startStatesLHS = {
		{1, lab},
	};

	return visitLHSUnlessError4Iterative(startStatesLHS, v);
}

bool IMMChecker::checkError4(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	if (visitUnlessError4(lab))
		return true;

	return visitError4(lab);
}

bool IMMChecker::visitError5([[maybe_unused]] const EventLabel *lab) const { return false; }

bool IMMChecker::visitLHSUnlessError5Iterative(std::vector<DFSWorklistEntry> &worklist) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		case 0: {
			if (isFinishing) {
				break;
			}

			cexLab = lab;
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
					cexLab = g.getEventLabelIfPresent(*tmpE);
					return false;
				}
			if (true && genmc::isa<HpRetireLabel>(lab))
				if (auto tmpE = g.free_pos(lab); tmpE) {
					cexLab = g.getEventLabelIfPresent(*tmpE);
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

bool IMMChecker::visitRHSUnlessError5Iterative(std::vector<DFSWorklistEntry> &worklist) const
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

bool IMMChecker::visitUnlessError5(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	/* Explore from all accepting states in LHS using DFS */
	std::vector<DFSWorklistEntry> startStatesLHS = {
		{1, lab},
	};

	if (!visitLHSUnlessError5Iterative(startStatesLHS))
		return false;

	return true;
}

bool IMMChecker::checkError5(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	if (visitUnlessError5(lab))
		return true;

	return visitError5(lab);
}

bool IMMChecker::visitError6([[maybe_unused]] const EventLabel *lab) const { return false; }

bool IMMChecker::visitLHSUnlessError6Iterative(std::vector<DFSWorklistEntry> &worklist,
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

bool IMMChecker::visitUnlessError6(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	auto &v = lab->view(0);

	/* Explore from all accepting states in LHS using DFS */
	std::vector<DFSWorklistEntry> startStatesLHS = {
		{1, lab},
	};

	return visitLHSUnlessError6Iterative(startStatesLHS, v);
}

bool IMMChecker::checkError6(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	if (visitUnlessError6(lab))
		return true;

	return visitError6(lab);
}

bool IMMChecker::visitError7([[maybe_unused]] const EventLabel *lab) const { return false; }

bool IMMChecker::visitLHSUnlessError7Iterative(std::vector<DFSWorklistEntry> &worklist) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		case 0: {
			if (isFinishing) {
				break;
			}

			cexLab = lab;
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
					cexLab = g.getEventLabelIfPresent(*tmpE);
					return false;
				}
			if (true && genmc::isa<WriteLabel>(lab))
				if (auto tmpE = g.free_pos(lab); tmpE) {
					cexLab = g.getEventLabelIfPresent(*tmpE);
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

bool IMMChecker::visitRHSUnlessError7Iterative(std::vector<DFSWorklistEntry> &worklist) const
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

bool IMMChecker::visitUnlessError7(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	/* Explore from all accepting states in LHS using DFS */
	std::vector<DFSWorklistEntry> startStatesLHS = {
		{1, lab},
	};

	if (!visitLHSUnlessError7Iterative(startStatesLHS))
		return false;

	return true;
}

bool IMMChecker::checkError7(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	if (visitUnlessError7(lab))
		return true;

	return visitError7(lab);
}

bool IMMChecker::visitError8([[maybe_unused]] const EventLabel *lab) const { return false; }

bool IMMChecker::visitLHSUnlessError8Iterative(std::vector<DFSWorklistEntry> &worklist,
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

bool IMMChecker::visitUnlessError8(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	auto &v = lab->view(0);

	/* Explore from all accepting states in LHS using DFS */
	std::vector<DFSWorklistEntry> startStatesLHS = {
		{1, lab},
	};

	return visitLHSUnlessError8Iterative(startStatesLHS, v);
}

bool IMMChecker::checkError8(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	if (visitUnlessError8(lab))
		return true;

	return visitError8(lab);
}

bool IMMChecker::visitError9([[maybe_unused]] const EventLabel *lab) const { return false; }

bool IMMChecker::visitLHSUnlessError9Iterative(std::vector<DFSWorklistEntry> &worklist) const
{
	while (!worklist.empty()) {
		auto [stateId, lab, isFinishing] = worklist.back();
		worklist.pop_back();
		switch (stateId) {
		case 0: {
			if (isFinishing) {
				break;
			}

			cexLab = lab;
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
					cexLab = g.getEventLabelIfPresent(*tmpE);
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

bool IMMChecker::visitRHSUnlessError9Iterative(std::vector<DFSWorklistEntry> &worklist) const
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

bool IMMChecker::visitUnlessError9(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	/* Explore from all accepting states in LHS using DFS */
	std::vector<DFSWorklistEntry> startStatesLHS = {
		{1, lab},
	};

	if (!visitLHSUnlessError9Iterative(startStatesLHS))
		return false;

	return true;
}

bool IMMChecker::checkError9(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	if (visitUnlessError9(lab))
		return true;

	return visitError9(lab);
}

bool IMMChecker::visitError10([[maybe_unused]] const EventLabel *lab) const { return false; }

bool IMMChecker::visitLHSUnlessError10Iterative(std::vector<DFSWorklistEntry> &worklist,
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

bool IMMChecker::visitUnlessError10(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	auto &v = lab->view(0);

	/* Explore from all accepting states in LHS using DFS */
	std::vector<DFSWorklistEntry> startStatesLHS = {
		{1, lab},
	};

	return visitLHSUnlessError10Iterative(startStatesLHS, v);
}

bool IMMChecker::checkError10(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	if (visitUnlessError10(lab))
		return true;

	return visitError10(lab);
}

bool IMMChecker::visitWarning11([[maybe_unused]] const EventLabel *lab) const { return false; }

bool IMMChecker::visitLHSUnlessWarning11Iterative(std::vector<DFSWorklistEntry> &worklist,
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

bool IMMChecker::visitUnlessWarning11(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	auto &v = lab->view(2);

	/* Explore from all accepting states in LHS using DFS */
	std::vector<DFSWorklistEntry> startStatesLHS = {
		{1, lab},
	};

	return visitLHSUnlessWarning11Iterative(startStatesLHS, v);
}

bool IMMChecker::checkWarning11(const EventLabel *lab) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	if (visitUnlessWarning11(lab))
		return true;

	return visitWarning11(lab);
}

std::optional<VerificationError>
IMMChecker::checkErrors([[maybe_unused]] const EventLabel *lab,
			[[maybe_unused]] const EventLabel *&race) const
{
	if (!checkError4(lab)) {
		race = cexLab;
		return {VerificationError::VE_AccessNonMalloc};
	}

	if (!checkError5(lab)) {
		race = cexLab;
		return {VerificationError::VE_DoubleFree};
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
		return {VerificationError::VE_AccessFreed};
	}

	if (!checkError10(lab)) {
		race = cexLab;
		return {VerificationError::VE_RaceNotAtomic};
	}

	return {};
}

std::vector<VerificationError>
IMMChecker::checkWarnings(const EventLabel *lab, const VSet<VerificationError> &seenWarnings,
			  std::vector<const EventLabel *> &racyLabs) const
{
	std::vector<VerificationError> result;

	if (seenWarnings.count(VerificationError::VE_WWRace) == 0 && !checkWarning11(lab)) {
		racyLabs.push_back(cexLab);
		result.push_back(VerificationError::VE_WWRace);
	}

	return result;
}

bool IMMChecker::isConsistent([[maybe_unused]] const EventLabel *lab) const
{

	return true && checkConsAcyclic1(lab) && checkConsAcyclic2(lab);
}

bool IMMChecker::isConsistent([[maybe_unused]] const ExecutionGraph &g) const
{

	return true && checkConsAcyclic1(g) && checkConsAcyclic2(g);
}

bool IMMChecker::isCoherentRelinche(const ExecutionGraph &g) const
{

	return true && visitCoherenceRelinche(g);
}

void IMMChecker::visitPPoRf0(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf0[lab->getStamp().get()] = NodeStatus::entered;
	pporf.updateIdx(lab->getPos());
	visitedPPoRf0[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitPPoRf1(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf1[lab->getStamp().get()] = NodeStatus::entered;
	for (auto &tmp : g.ctrl_preds(lab))
		if (auto *pLab = &tmp; true)
			if (true && pLab->isDependable())
				if (pporf.updateIdx(pLab->getPos()); true) {
					auto status = visitedPPoRf0[pLab->getStamp().get()];
					if (status == NodeStatus::unseen)
						visitPPoRf0(pLab, pporf);
				}
	for (auto &tmp : g.addr_preds(lab))
		if (auto *pLab = &tmp; true)
			if (true && pLab->isDependable())
				if (pporf.updateIdx(pLab->getPos()); true) {
					auto status = visitedPPoRf0[pLab->getStamp().get()];
					if (status == NodeStatus::unseen)
						visitPPoRf0(pLab, pporf);
				}
	for (auto &tmp : g.data_preds(lab))
		if (auto *pLab = &tmp; true)
			if (true && pLab->isDependable())
				if (pporf.updateIdx(pLab->getPos()); true) {
					auto status = visitedPPoRf0[pLab->getStamp().get()];
					if (status == NodeStatus::unseen)
						visitPPoRf0(pLab, pporf);
				}
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
	if (auto pLab = g.rfi_pred(lab); pLab) {
		auto status = visitedPPoRf1[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf1(pLab, pporf);
	}
	if (auto pLab = g.rfi_pred(lab); pLab)
		if (true && genmc::isa<WriteLabel>(pLab) &&
		    ((genmc::isa<ReadLabel>(pLab) && genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
		     (genmc::isa<WriteLabel>(pLab) &&
		      genmc::dyn_cast<WriteLabel>(pLab)->isRMW()))) {
			auto status = visitedPPoRf2[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf2(pLab, pporf);
		}
	for (auto &tmp : g.ctrl_preds(lab))
		if (auto *pLab = &tmp; true)
			if (true && pLab->isDependable()) {
				auto status = visitedPPoRf3[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf3(pLab, pporf);
			}
	for (auto &tmp : g.addr_preds(lab))
		if (auto *pLab = &tmp; true)
			if (true && pLab->isDependable()) {
				auto status = visitedPPoRf3[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf3(pLab, pporf);
			}
	for (auto &tmp : g.data_preds(lab))
		if (auto *pLab = &tmp; true)
			if (true && pLab->isDependable()) {
				auto status = visitedPPoRf3[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf3(pLab, pporf);
			}
	for (auto &tmp : g.ctrl_preds(lab))
		if (auto *pLab = &tmp; true)
			if (true && pLab->isDependable()) {
				auto status = visitedPPoRf5[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf5(pLab, pporf);
			}
	for (auto &tmp : g.addr_preds(lab))
		if (auto *pLab = &tmp; true)
			if (true && pLab->isDependable()) {
				auto status = visitedPPoRf5[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf5(pLab, pporf);
			}
	for (auto &tmp : g.data_preds(lab))
		if (auto *pLab = &tmp; true)
			if (true && pLab->isDependable()) {
				auto status = visitedPPoRf5[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf5(pLab, pporf);
			}
	visitedPPoRf1[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitPPoRf2(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf2[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<ReadLabel>(pLab) &&
		    ((genmc::isa<ReadLabel>(pLab) && genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
		     (genmc::isa<WriteLabel>(pLab) &&
		      genmc::dyn_cast<WriteLabel>(pLab)->isRMW())) &&
		    pLab->isDependable())
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf0[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf0(pLab, pporf);
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
		if (true && genmc::isa<ReadLabel>(pLab) &&
		    ((genmc::isa<ReadLabel>(pLab) && genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
		     (genmc::isa<WriteLabel>(pLab) &&
		      genmc::dyn_cast<WriteLabel>(pLab)->isRMW())) &&
		    pLab->isDependable()) {
			auto status = visitedPPoRf3[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf3(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<ReadLabel>(pLab) &&
		    ((genmc::isa<ReadLabel>(pLab) && genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
		     (genmc::isa<WriteLabel>(pLab) &&
		      genmc::dyn_cast<WriteLabel>(pLab)->isRMW())) &&
		    pLab->isDependable()) {
			auto status = visitedPPoRf5[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf5(pLab, pporf);
		}
	visitedPPoRf2[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitPPoRf3(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf3[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.tc_pred(lab); pLab)
		if (pporf.updateIdx(pLab->getPos()); true) {
			auto status = visitedPPoRf0[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf0(pLab, pporf);
		}
	if (auto pLab = g.tj_pred(lab); pLab)
		if (pporf.updateIdx(pLab->getPos()); true) {
			auto status = visitedPPoRf0[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf0(pLab, pporf);
		}
	for (auto &tmp : g.lin_preds(lab))
		if (auto *pLab = &tmp; true)
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf0[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf0(pLab, pporf);
			}
	if (auto pLab = g.rfe_pred(lab); pLab)
		if (pporf.updateIdx(pLab->getPos()); true) {
			auto status = visitedPPoRf0[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf0(pLab, pporf);
		}
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.ctrl_preds(lab))
			if (auto *pLab = &tmp; true)
				if (true && pLab->isDependable())
					if (pporf.updateIdx(pLab->getPos()); true) {
						auto status = visitedPPoRf0[pLab->getStamp().get()];
						if (status == NodeStatus::unseen)
							visitPPoRf0(pLab, pporf);
					}
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.addr_preds(lab))
			if (auto *pLab = &tmp; true)
				if (true && pLab->isDependable())
					if (pporf.updateIdx(pLab->getPos()); true) {
						auto status = visitedPPoRf0[pLab->getStamp().get()];
						if (status == NodeStatus::unseen)
							visitPPoRf0(pLab, pporf);
					}
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.data_preds(lab))
			if (auto *pLab = &tmp; true)
				if (true && pLab->isDependable())
					if (pporf.updateIdx(pLab->getPos()); true) {
						auto status = visitedPPoRf0[pLab->getStamp().get()];
						if (status == NodeStatus::unseen)
							visitPPoRf0(pLab, pporf);
					}
	for (auto &tmp : g.detour_preds(lab))
		if (auto *pLab = &tmp; true)
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf0[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf0(pLab, pporf);
			}
	if (true && genmc::isa<WriteLabel>(lab))
		if (auto pLab = g.poloc_imm_pred(lab); pLab)
			if (true && pLab->isAtLeastRelease() && genmc::isa<WriteLabel>(pLab))
				if (pporf.updateIdx(pLab->getPos()); true) {
					auto status = visitedPPoRf0[pLab->getStamp().get()];
					if (status == NodeStatus::unseen)
						visitPPoRf0(pLab, pporf);
				}
	if (true && genmc::isa<WriteLabel>(lab) &&
	    ((genmc::isa<ReadLabel>(lab) && genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
	     (genmc::isa<WriteLabel>(lab) && genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<ReadLabel>(pLab) &&
			    ((genmc::isa<ReadLabel>(pLab) &&
			      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
			     (genmc::isa<WriteLabel>(pLab) &&
			      genmc::dyn_cast<WriteLabel>(pLab)->isRMW())) &&
			    pLab->isDependable())
				if (pporf.updateIdx(pLab->getPos()); true) {
					auto status = visitedPPoRf0[pLab->getStamp().get()];
					if (status == NodeStatus::unseen)
						visitPPoRf0(pLab, pporf);
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
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.data_preds(lab))
			if (auto *pLab = &tmp; true) {
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
	if (auto pLab = g.tc_pred(lab); pLab) {
		auto status = visitedPPoRf3[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf3(pLab, pporf);
	}
	if (auto pLab = g.tj_pred(lab); pLab) {
		auto status = visitedPPoRf3[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf3(pLab, pporf);
	}
	for (auto &tmp : g.lin_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf3[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf3(pLab, pporf);
		}
	if (auto pLab = g.rfe_pred(lab); pLab) {
		auto status = visitedPPoRf3[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf3(pLab, pporf);
	}
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.ctrl_preds(lab))
			if (auto *pLab = &tmp; true)
				if (true && pLab->isDependable()) {
					auto status = visitedPPoRf3[pLab->getStamp().get()];
					if (status == NodeStatus::unseen)
						visitPPoRf3(pLab, pporf);
				}
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.addr_preds(lab))
			if (auto *pLab = &tmp; true)
				if (true && pLab->isDependable()) {
					auto status = visitedPPoRf3[pLab->getStamp().get()];
					if (status == NodeStatus::unseen)
						visitPPoRf3(pLab, pporf);
				}
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.data_preds(lab))
			if (auto *pLab = &tmp; true)
				if (true && pLab->isDependable()) {
					auto status = visitedPPoRf3[pLab->getStamp().get()];
					if (status == NodeStatus::unseen)
						visitPPoRf3(pLab, pporf);
				}
	for (auto &tmp : g.detour_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf3[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf3(pLab, pporf);
		}
	if (true && genmc::isa<WriteLabel>(lab))
		if (auto pLab = g.poloc_imm_pred(lab); pLab)
			if (true && pLab->isAtLeastRelease() && genmc::isa<WriteLabel>(pLab)) {
				auto status = visitedPPoRf3[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf3(pLab, pporf);
			}
	if (true && lab->isAtLeastRelease())
		if (auto pLab = g.po_imm_pred(lab); pLab) {
			auto status = visitedPPoRf3[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf3(pLab, pporf);
		}
	if (true && genmc::isa<WriteLabel>(lab) &&
	    ((genmc::isa<ReadLabel>(lab) && genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
	     (genmc::isa<WriteLabel>(lab) && genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<ReadLabel>(pLab) &&
			    ((genmc::isa<ReadLabel>(pLab) &&
			      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
			     (genmc::isa<WriteLabel>(pLab) &&
			      genmc::dyn_cast<WriteLabel>(pLab)->isRMW())) &&
			    pLab->isDependable()) {
				auto status = visitedPPoRf3[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf3(pLab, pporf);
			}
	if (true && genmc::isa<FenceLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab) {
			auto status = visitedPPoRf3[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf3(pLab, pporf);
		}
	if (true && genmc::isa<ThreadJoinLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab) {
			auto status = visitedPPoRf3[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf3(pLab, pporf);
		}
	if (true && lab->isAtLeastRelease())
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf4[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf4(pLab, pporf);
			}
	if (true && genmc::isa<FenceLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf4[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf4(pLab, pporf);
			}
	if (true && genmc::isa<ThreadJoinLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf4[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf4(pLab, pporf);
			}
	if (auto pLab = g.tc_pred(lab); pLab) {
		auto status = visitedPPoRf5[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf5(pLab, pporf);
	}
	if (auto pLab = g.tj_pred(lab); pLab) {
		auto status = visitedPPoRf5[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf5(pLab, pporf);
	}
	for (auto &tmp : g.lin_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf5[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf5(pLab, pporf);
		}
	if (auto pLab = g.rfe_pred(lab); pLab) {
		auto status = visitedPPoRf5[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf5(pLab, pporf);
	}
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.ctrl_preds(lab))
			if (auto *pLab = &tmp; true)
				if (true && pLab->isDependable()) {
					auto status = visitedPPoRf5[pLab->getStamp().get()];
					if (status == NodeStatus::unseen)
						visitPPoRf5(pLab, pporf);
				}
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.addr_preds(lab))
			if (auto *pLab = &tmp; true)
				if (true && pLab->isDependable()) {
					auto status = visitedPPoRf5[pLab->getStamp().get()];
					if (status == NodeStatus::unseen)
						visitPPoRf5(pLab, pporf);
				}
	if (true && genmc::isa<WriteLabel>(lab))
		for (auto &tmp : g.data_preds(lab))
			if (auto *pLab = &tmp; true)
				if (true && pLab->isDependable()) {
					auto status = visitedPPoRf5[pLab->getStamp().get()];
					if (status == NodeStatus::unseen)
						visitPPoRf5(pLab, pporf);
				}
	for (auto &tmp : g.detour_preds(lab))
		if (auto *pLab = &tmp; true) {
			auto status = visitedPPoRf5[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf5(pLab, pporf);
		}
	if (true && genmc::isa<WriteLabel>(lab))
		if (auto pLab = g.poloc_imm_pred(lab); pLab)
			if (true && pLab->isAtLeastRelease() && genmc::isa<WriteLabel>(pLab)) {
				auto status = visitedPPoRf5[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf5(pLab, pporf);
			}
	if (true && lab->isAtLeastRelease())
		if (auto pLab = g.po_imm_pred(lab); pLab) {
			auto status = visitedPPoRf5[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf5(pLab, pporf);
		}
	if (true && genmc::isa<WriteLabel>(lab) &&
	    ((genmc::isa<ReadLabel>(lab) && genmc::dyn_cast<ReadLabel>(lab)->isRMW()) ||
	     (genmc::isa<WriteLabel>(lab) && genmc::dyn_cast<WriteLabel>(lab)->isRMW())))
		if (auto pLab = g.po_imm_pred(lab); pLab)
			if (true && genmc::isa<ReadLabel>(pLab) &&
			    ((genmc::isa<ReadLabel>(pLab) &&
			      genmc::dyn_cast<ReadLabel>(pLab)->isRMW()) ||
			     (genmc::isa<WriteLabel>(pLab) &&
			      genmc::dyn_cast<WriteLabel>(pLab)->isRMW())) &&
			    pLab->isDependable()) {
				auto status = visitedPPoRf5[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf5(pLab, pporf);
			}
	if (true && genmc::isa<FenceLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab) {
			auto status = visitedPPoRf5[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf5(pLab, pporf);
		}
	if (true && genmc::isa<ThreadJoinLabel>(lab))
		if (auto pLab = g.po_imm_pred(lab); pLab) {
			auto status = visitedPPoRf5[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf5(pLab, pporf);
		}
	if (true && genmc::isa<WriteLabel>(lab))
		if (auto pLab = g.poloc_imm_pred(lab); pLab) {
			auto status = visitedPPoRf6[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf6(pLab, pporf);
		}
	visitedPPoRf3[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitPPoRf4(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf4[lab->getStamp().get()] = NodeStatus::entered;
	pporf.updateIdx(lab->getPos());
	if (auto pLab = g.po_imm_pred(lab); pLab) {
		auto status = visitedPPoRf3[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf3(pLab, pporf);
	}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (pporf.updateIdx(pLab->getPos()); true) {
			auto status = visitedPPoRf4[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf4(pLab, pporf);
		}
	visitedPPoRf4[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitPPoRf5(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf5[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && pLab->isAtLeastAcquire())
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf0[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf0(pLab, pporf);
			}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<FenceLabel>(pLab))
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf0[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf0(pLab, pporf);
			}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<ThreadJoinLabel>(pLab))
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf0[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf0(pLab, pporf);
			}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && pLab->isAtLeastAcquire()) {
			auto status = visitedPPoRf3[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf3(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<FenceLabel>(pLab)) {
			auto status = visitedPPoRf3[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf3(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<ThreadJoinLabel>(pLab)) {
			auto status = visitedPPoRf3[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf3(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab) {
		auto status = visitedPPoRf5[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf5(pLab, pporf);
	}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && pLab->isAtLeastAcquire()) {
			auto status = visitedPPoRf5[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf5(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<FenceLabel>(pLab)) {
			auto status = visitedPPoRf5[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf5(pLab, pporf);
		}
	if (auto pLab = g.po_imm_pred(lab); pLab)
		if (true && genmc::isa<ThreadJoinLabel>(pLab)) {
			auto status = visitedPPoRf5[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf5(pLab, pporf);
		}
	visitedPPoRf5[lab->getStamp().get()] = NodeStatus::left;
}

void IMMChecker::visitPPoRf6(const EventLabel *lab, DepView &pporf) const
{
	[[maybe_unused]] auto &g = *lab->getParent();

	visitedPPoRf6[lab->getStamp().get()] = NodeStatus::entered;
	if (auto pLab = g.poloc_imm_pred(lab); pLab)
		if (true && pLab->isAtLeastRelease() && genmc::isa<WriteLabel>(pLab))
			if (pporf.updateIdx(pLab->getPos()); true) {
				auto status = visitedPPoRf0[pLab->getStamp().get()];
				if (status == NodeStatus::unseen)
					visitPPoRf0(pLab, pporf);
			}
	if (auto pLab = g.poloc_imm_pred(lab); pLab)
		if (true && pLab->isAtLeastRelease() && genmc::isa<WriteLabel>(pLab)) {
			auto status = visitedPPoRf3[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf3(pLab, pporf);
		}
	if (auto pLab = g.poloc_imm_pred(lab); pLab)
		if (true && pLab->isAtLeastRelease() && genmc::isa<WriteLabel>(pLab)) {
			auto status = visitedPPoRf5[pLab->getStamp().get()];
			if (status == NodeStatus::unseen)
				visitPPoRf5(pLab, pporf);
		}
	if (auto pLab = g.poloc_imm_pred(lab); pLab) {
		auto status = visitedPPoRf6[pLab->getStamp().get()];
		if (status == NodeStatus::unseen)
			visitPPoRf6(pLab, pporf);
	}
	visitedPPoRf6[lab->getStamp().get()] = NodeStatus::left;
}

DepView IMMChecker::calcPPoRfBefore(const EventLabel *lab) const
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

	visitPPoRf3(lab, pporf);
	visitPPoRf5(lab, pporf);
	return pporf;
}

std::unique_ptr<VectorClock> IMMChecker::calculatePrefixView(const EventLabel *lab) const
{
	return std::make_unique<DepView>(calcPPoRfBefore(lab));
}

std::unique_ptr<VectorClock> IMMChecker::calculateRevisitPrefixView(const EventLabel *lab) const
{
	return calculatePrefixView(lab);
}

void IMMChecker::recomputeCacheCounters([[maybe_unused]] const ExecutionGraph &g) const
{
	recomputeUnlessConsAcyclic1(g);
}

void IMMChecker::resetCacheCounters() const { cacheCounterUnlessConsAcyclic1 = -1; }

void IMMChecker::maybeDecreaseCacheCounters([[maybe_unused]] const EventLabel *lab) const
{
	if (true && lab->isSC()) {
		VERIFY(cacheCounterUnlessConsAcyclic1 > 0);
		cacheCounterUnlessConsAcyclic1 -= 1;
	}
}

void IMMChecker::maybeIncreaseCacheCounters([[maybe_unused]] const EventLabel *lab) const
{
	if (true && lab->isSC()) {
		VERIFY(cacheCounterUnlessConsAcyclic1 >= 0);
		cacheCounterUnlessConsAcyclic1 += 1;
	}
}

// NOLINTEND
