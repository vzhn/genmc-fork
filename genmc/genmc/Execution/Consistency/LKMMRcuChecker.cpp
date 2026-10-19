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
#include "genmc/Verification/Config.hpp"

auto LKMMRcuChecker::withModel(const ExecutionGraph &g, bool generated) const -> bool
{
	const bool reference = getConf()->lkmmReference;
	if (!reference && (!generated || !lkmm::hasGracePeriod(g)))
		return generated;

	auto v = lkmm::evaluate(g);
	/* The generated checker decides hb and pb (coherence and atomicity hold
	 * by construction); the graphs it is asked about were consistent before
	 * their last event */
	if (reference && (generated != (v.hb && v.pb) || !v.coherence || !v.atomicity))
		ERROR("The LKMM checker and the model disagree (checker: {}, model: "
		      "coherence {}, atomicity {}, hb {}, pb {}) on:\n{}\n",
		      generated ? "consistent" : "inconsistent", v.coherence, v.atomicity,
		      v.hb, v.pb, g);
	return generated && v.rcu;
}

/* Whether adding LAB can make rb reflexive: not when LAB is the last event
 * of its thread and has no successor (a read of the co-maximal write, a
 * co-maximal write, an event that is neither), as an rb cycle through it
 * has to leave it; but closing a critical section orders the events
 * already in it */
static auto mayCloseRcuCycle(const EventLabel *lab) -> bool
{
	const auto &g = *lab->getParent();
	if (g.getLastThreadLabel(lab->getThread()) != lab)
		return true;
	if (const auto *fLab = genmc::dyn_cast<LkmmFenceLabel>(lab))
		return fLab->getFenceKind() == LkmmFenceKind::RcuUnlock;
	if (const auto *rLab = genmc::dyn_cast<ReadLabel>(lab))
		return rLab->getRf() != g.co_max(rLab->getAddr());
	if (const auto *wLab = genmc::dyn_cast<WriteLabel>(lab))
		return wLab != g.co_max(wLab->getAddr());
	return false;
}

auto LKMMRcuChecker::isConsistent(const EventLabel *lab) const -> bool
{
	auto generated = [&] {
		lkmm::PropIntCache cache;
		return LKMMChecker::isConsistent(lab);
	}();
	if (!getConf()->lkmmReference && !mayCloseRcuCycle(lab))
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
	       (g.getLastThreadLabel(rLab->getThread()) == rLab || !lkmm::hasGracePeriod(g));
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

	/* An access added after the free is not before it (without RCU) */
	if (genmc::isa<ReadLabel>(lab) || genmc::isa<WriteLabel>(lab))
		if (auto pos = g.free_pos(lab); pos) {
			const auto *fLab = g.getEventLabel(*pos);
			if (!lkmm::hasGracePeriod(g) || !lkmm::isRcuOrderedBefore(g, lab, fLab)) {
				race = fLab;
				return {VerificationError::VE_AccessFreed};
			}
		}
	return {};
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
