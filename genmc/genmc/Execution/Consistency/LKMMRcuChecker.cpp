/*
 * GenMC -- Generic Model Checking.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "genmc/Execution/Consistency/LKMMRcuChecker.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Execution/LkmmModel.hpp"
#include "genmc/Execution/LkmmRelations.hpp"
#include "genmc/Support/Error.hpp"

#include <algorithm>

/* Whether G has what the generated checker cannot decide: a grace period
 * (rb) or a potential race (plain-coherence) */
static auto needsModel(const ExecutionGraph &g) -> bool
{
	return lkmm::hasGracePeriod(g) || lkmm::hasPreRace(g);
}

auto LKMMRcuChecker::withModel(const ExecutionGraph &g, bool generated) const -> bool
{
	const bool reference = getConf()->lkmmReference;
	if (!reference && (!generated || !needsModel(g)))
		return generated;

	auto v = lkmm::evaluate(g, reference);
	/* The generated checker decides hb and pb (coherence and atomicity hold
	 * by construction); the graphs it is asked about were consistent before
	 * their last event */
	if (reference && (generated != (v.hb && v.pb) || !v.coherence || !v.atomicity))
		ERROR("The LKMM checker and the model disagree (checker: {}, model: "
		      "coherence {}, atomicity {}, hb {}, pb {}) on:\n{}\n",
		      generated ? "consistent" : "inconsistent", v.coherence, v.atomicity,
		      v.hb, v.pb, g);
	return generated && v.rcu && v.plainCoherence;
}

/* Whether adding LAB can make rb reflexive or break plain-coherence: not
 * when LAB is the last event of its thread and has no successor (a read of
 * the co-maximal write, a co-maximal write, an event that is neither), as
 * an rb cycle, or a pair a plain-coherence rule forbids, has to leave it;
 * but closing a critical section (rcu_read_unlock(), srcu_read_unlock())
 * orders the events already in it */
static auto mayBreakModel(const EventLabel *lab) -> bool
{
	const auto &g = *lab->getParent();
	if (g.getLastThreadLabel(lab->getThread()) != lab)
		return true;
	if (const auto *fLab = genmc::dyn_cast<LkmmFenceLabel>(lab))
		return fLab->getFenceKind() == LkmmFenceKind::RcuUnlock;
	if (genmc::isa<SrcuUnlockWriteLabel>(lab))
		return true;
	if (const auto *rLab = genmc::dyn_cast<ReadLabel>(lab))
		return rLab->getRf() != g.co_max(rLab->getAddr());
	if (const auto *wLab = genmc::dyn_cast<WriteLabel>(lab))
		return wLab != g.co_max(wLab->getAddr());
	return false;
}

auto LKMMRcuChecker::isConsistent(const EventLabel *lab) const -> bool
{
	const auto &g = *lab->getParent();
	/* A plain access is no event of hb, pb or rb, and its rf and co edges
	 * are no steps of them: they relate marked events (prop starts and ends
	 * in one), and carry-dep reaches a plain read only by an rfi, before
	 * the read has dependents (whose own checks follow). Of the axioms,
	 * only coherence, which GenMC's choices of rf and co keep, and
	 * plain-coherence concern it */
	if (!getConf()->lkmmReference && genmc::isa<MemAccessLabel>(lab) && lab->isNotAtomic())
		return !mayBreakModel(lab) || !lkmm::hasPreRace(g) ||
		       lkmm::evaluate(g, false).plainCoherence;

	/* A fence at the end of its thread has no successor in the regular
	 * part of the model: only RCU can make it inconsistent (an unlock
	 * closing a critical section) */
	auto generated = (!getConf()->lkmmReference && genmc::isa<FenceLabel>(lab) &&
			  g.getLastThreadLabel(lab->getThread()) == lab) ||
			 [&] {
				 lkmm::PropIntCache cache;
				 return LKMMChecker::isConsistent(lab);
			 }();
	if (!getConf()->lkmmReference && !mayBreakModel(lab))
		return generated;
	return withModel(*lab->getParent(), generated);
}

auto LKMMRcuChecker::isConsistent(const ExecutionGraph &g) const -> bool
{
	auto generated = [&] {
		lkmm::PropIntCache cache;
		return LKMMChecker::isConsistent(g);
	}();
	return withModel(g, generated);
}

auto LKMMRcuChecker::isCoMaxReadConsistent(const ReadLabel *rLab) const -> bool
{
	const auto &g = *rLab->getParent();
	return !getConf()->lkmmReference &&
	       (g.getLastThreadLabel(rLab->getThread()) == rLab || !needsModel(g));
}

auto LKMMRcuChecker::checkAccessFreed(const EventLabel *lab, const EventLabel *&race) const
	-> std::optional<VerificationError>
{
	const auto &g = *lab->getParent();
	auto isBefore = [&](const EventLabel *a, const EventLabel *b) {
		return getHbView(b).contains(a->getPos()) ||
		       (lkmm::hasGracePeriod(g) && lkmm::isRcuOrderedBefore(g, a, b));
	};

	/* The last accesses of each thread are before the free */
	if (genmc::isa<FreeLabel>(lab) && !genmc::isa<HpRetireLabel>(lab)) {
		for (const auto &view : {g.pomax_at_reads_pos(lab), g.pomax_at_writes_pos(lab),
					 g.pomax_na_reads_pos(lab), g.pomax_na_writes_pos(lab)})
			for (auto e : view)
				if (const auto *a = g.getEventLabelIfPresent(e); a && !isBefore(a, lab)) {
					race = a;
					return {VerificationError::VE_AccessFreed};
				}
	}

	/* An access added after the free is not before it (without RCU), unless
	 * the free is a kept event that the access's thread reaches again */
	if (genmc::isa<ReadLabel>(lab) || genmc::isa<WriteLabel>(lab))
		if (auto pos = g.free_pos(lab); pos) {
			const auto *fLab = g.getEventLabel(*pos);
			if (!isBefore(lab, fLab)) {
				race = fLab;
				return {VerificationError::VE_AccessFreed};
			}
		}
	return {};
}

auto LKMMRcuChecker::checkWarnings(const EventLabel *lab, const VSet<VerificationError> &reported,
				   std::vector<const EventLabel *> &races) const
	-> std::vector<VerificationError>
{
	auto warnings = LKMMChecker::checkWarnings(lab, reported, races);
	auto it = std::ranges::find(warnings, VerificationError::VE_WWRace);
	const auto &g = *lab->getParent();
	if (it == warnings.end() || !lkmm::hasGracePeriod(g))
		return warnings;

	/* The rule found a last marked write of another thread outside the hb
	 * view of LAB. Take the writes RCU orders, either way, as ordered: the
	 * rule's counterexample is one of them, the others may not be */
	auto idx = it - warnings.begin();
	auto rcuOrdered = [&](const EventLabel *a, const EventLabel *b) {
		return lkmm::isRcuOrderedBefore(g, a, b) || lkmm::isRcuOrderedBefore(g, b, a);
	};
	const EventLabel *racy = nullptr;
	for (auto e : g.pomax_at_writes_pos(lab)) {
		const auto *wLab = g.getEventLabelIfPresent(e);
		if (wLab && !getHbView(lab).contains(e) && !rcuOrdered(wLab, lab)) {
			racy = wLab;
			break;
		}
	}
	if (racy) {
		races[idx] = racy;
	} else {
		warnings.erase(it);
		races.erase(races.begin() + idx);
	}
	return warnings;
}

auto LKMMRcuChecker::checkErrors(const EventLabel *lab, const EventLabel *&race) const
	-> std::optional<VerificationError>
{
	/* In the order of the kat's rules: a non-malloc'ed access and a double
	 * free come before an access of freed memory */
	auto err = LKMMChecker::checkErrors(lab, race);
	if (err == VerificationError::VE_AccessNonMalloc || err == VerificationError::VE_DoubleFree)
		return err;
	if (auto freed = checkAccessFreed(lab, race); freed)
		return freed;
	return err;
}
