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

#ifndef GENMC_LKMM_RCU_CHECKER_HPP
#define GENMC_LKMM_RCU_CHECKER_HPP

#include "genmc/Execution/Consistency/LKMMChecker.hpp"

/** The Linux-kernel memory model: the Kater-generated LKMMChecker, plus the
 * axioms it cannot generate: RCU's (irreflexive rb), which counts grace
 * periods against critical sections and is no regular relation, and
 * plain-coherence, which intersects relations. A direct evaluation of the
 * model (LkmmModel.cpp) decides them for the graphs that have a
 * synchronize_rcu() or a potential race; with -lkmm-reference it checks
 * every graph, and stops at the first one where it and the generated
 * checker disagree. Data races are found on complete executions (the
 * driver asks lkmm::findRace()). The checks of an access
 * against a free of its memory are here too, as a grace period orders the
 * accesses of a reader before a kfree() after synchronize_rcu() */
class LKMMRcuChecker : public LKMMChecker {
public:
	LKMMRcuChecker(const Config *conf) : LKMMChecker(conf) {}

	auto isConsistent(const EventLabel *lab) const -> bool override;
	auto isConsistent(const ExecutionGraph &g) const -> bool override;

	/* rcu-link and rcu-fence order by po, not ppo: a read of the co-maximal
	 * write that a revisit leaves before events of its thread (an
	 * rcu_read_unlock() among them) can close an rb cycle. A read at the
	 * end of its thread cannot */
	auto isCoMaxReadConsistent(const ReadLabel *rLab) const -> bool override;

	auto checkErrors(const EventLabel *lab, const EventLabel *&race) const
		-> std::optional<VerificationError> override;

private:
	/* An access of freed memory: last_accesses ; [FREE] <= hb and
	 * free ; [R | W] = 0, with RCU's ordering as well as hb */
	auto checkAccessFreed(const EventLabel *lab, const EventLabel *&race) const
		-> std::optional<VerificationError>;

	/* Combines the generated checker's answer on G with the model's */
	auto withModel(const ExecutionGraph &g, bool generated) const -> bool;
};

#endif /* GENMC_LKMM_RCU_CHECKER_HPP */
