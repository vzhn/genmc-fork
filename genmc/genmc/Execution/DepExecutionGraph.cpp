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

#include "DepExecutionGraph.hpp"
#include "genmc/ADT/DepView.hpp"
#include "genmc/ADT/VectorClock.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Execution/Stamp.hpp"
#include "genmc/Support/Cast.hpp"
#include "genmc/Support/Error.hpp"

#include <memory>
#include <ranges>

auto DepExecutionGraph::getViewFromStamp(Stamp stamp) const -> std::unique_ptr<VectorClock>
{
	auto preds = std::make_unique<DepView>();

	for (auto i = 0; i < getNumThreads(); i++) {
		for (auto j = 1; j < getThreadSize(i); j++) {
			const EventLabel *lab = getEventLabel(Event(i, j));
			if (lab->getStamp() <= stamp)
				preds->setMax(Event(i, j));
		}
	}
	return preds;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void DepExecutionGraph::cutToStamp(Stamp stamp)
{
	/* First remove events from the modification order */
	auto preds = getViewFromStamp(stamp);

	/* Inform all calculators about the events cutted */
	removeAfter(*preds);
	for (auto labIt = insertionOrder.begin(); labIt != insertionOrder.end();) {
		if (labIt->getIndex() > preds->getMax(labIt->getThread()))
			poLists[labIt->getThread()].remove(*labIt);
		labIt = preds->contains(labIt->getPos()) ? ++labIt : insertionOrder.erase(labIt);
	}

	/* Then, restrict the graph */
	for (auto i = 0; i < getNumThreads(); i++) {
		for (auto j = 0; j <= preds->getMax(i); j++) { /* Keeps begins */
			auto *lab = getEventLabel(Event(i, j));
			if (!preds->contains(lab->getPos()))
				continue;

			/* Otherwise, remove 'pointers' to it, in an
			 * analogous manner cutToView(). */
			if (auto *wLab = genmc::dyn_cast<WriteLabel>(lab)) {
				wLab->removeReader([&](ReadLabel &rLab) {
					return !preds->contains(rLab.getPos());
				});
			}
			if (auto *rLab = genmc::dyn_cast<ReadLabel>(lab)) {
				if (rLab->getRf() && !preds->contains(rLab->getRf()->getPos()))
					rLab->setRfNoCascade(nullptr);
			}
			if (auto *tsLab = genmc::dyn_cast<ThreadStartLabel>(lab)) {
				if (tsLab->getCreate() &&
				    !preds->contains(tsLab->getCreate()->getPos()))
					tsLab->setCreate(nullptr);
			}
			if (auto *eLab = genmc::dyn_cast<ThreadFinishLabel>(lab)) {
				if (eLab->getParentJoin() &&
				    !preds->contains(eLab->getParentJoin()->getPos()))
					eLab->setParentJoin(nullptr);
			}
			if (auto *begLab = genmc::dyn_cast<MethodBeginLabel>(lab)) {
				begLab->removePredNoCascade([&](auto *endLab) {
					return !preds->contains(endLab->getPos());
				});
			}
			if (auto *endLab = genmc::dyn_cast<MethodEndLabel>(lab)) {
				endLab->removeSuccNoCascade([&](auto *begLab) {
					return !preds->contains(begLab->getPos());
				});
			}
		}
	}

	/* Restrict the graph according to the view (keep begins around) */
	for (auto i = 0; i < getNumThreads(); i++) {
		auto &thr = events[i];
		thr.erase(thr.begin() + preds->getMax(i) + 1, thr.end());
	}

	/* Remove begins as well */
	removeOrphanBegins();

	/* Fix stamps */
	resetStamp(0U);
	for (auto &lab : labels())
		lab.setStamp(nextStamp());

	/* Finally, do not keep any nullptrs in the graph */
	for (auto i = 0; i < getNumThreads(); i++) {
		for (auto j = 0; j < getThreadSize(i); j++) {
			if (preds->contains(Event(i, j)))
				continue;
			auto it = po_iterator(getEventLabel(Event(i, j)));
			it = poLists[i].erase(it);
			events[i][j] = createHoleLabel(Event(i, j));
			getEventLabel(Event(i, j))->setStamp(nextStamp());
			getEventLabel(Event(i, j))->setParent(this);
			insertionOrder.push_back(*getEventLabel(Event(i, j)));
			poLists[i].insert(it, *getEventLabel(Event(i, j)));
		}
	}

	/* A hole stands for an event before ones a revisit kept. The view's last
	 * event of a thread can be one the view does not contain: the hole it
	 * becomes would be the thread's last label, with nothing kept after it,
	 * and the thread's next event, which goes to its slot, would then follow
	 * holes that stand for nothing (#180). Such holes go */
	auto trimmed = false;
	for (auto i = 0; i < getNumThreads(); i++) {
		auto &thr = events[i];
		while (!thr.empty() && genmc::isa<EmptyLabel>(&*thr.back())) {
			auto &hole = *thr.back();
			poLists[i].remove(hole);
			insertionOrder.remove(hole);
			thr.pop_back();
			trimmed = true;
		}
	}
	/* (and the stamps stay dense) */
	if (trimmed) {
		resetStamp(0U);
		for (auto &lab : labels())
			lab.setStamp(nextStamp());
	}
}

auto DepExecutionGraph::getCopyUpTo(const VectorClock &v) const -> std::unique_ptr<ExecutionGraph>
{
	auto og = std::make_unique<DepExecutionGraph>(
		ExecutionGraph::Config{.execState = this->state_,
				       .memModel = this->memModel_,
				       .emitNALabels = this->haveNAs_,
				       .allowUninitReads = this->allowUninitReads_});
	copyGraphUpTo(*og, v);
	return og;
}
