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

#ifndef GENMC_LKMM_MODEL_HPP
#define GENMC_LKMM_MODEL_HPP

#include <optional>
#include <string>

class EventLabel;
class ExecutionGraph;

/** A direct evaluation of the Linux-kernel memory model on a whole graph,
 * with relations as bit matrices. It decides RCU's axiom, which Kater cannot
 * generate, and serves as a reference for the generated LKMMChecker */
namespace lkmm {

/** Which of the model's axioms a graph satisfies */
struct Verdict {
	bool coherence = true; /* acyclic po-loc | com */
	bool atomicity = true; /* empty rmw & (fre ; coe) */
	bool hb = true;	       /* acyclic hb */
	bool pb = true;	       /* acyclic pb */
	bool rcu = true;       /* irreflexive rb */
	bool plainCoherence = true; /* empty (wr-incoh | rw-incoh | ww-incoh) */

	[[nodiscard]] auto consistent() const -> bool
	{
		return coherence && atomicity && hb && pb && rcu && plainCoherence;
	}
};

/** A data race: two accesses, and which of ww-race, wr-race, rw-race */
struct Race {
	const EventLabel *a = nullptr;
	const EventLabel *b = nullptr;
	std::string kind;
};

/** Whether G has a synchronize_rcu(): without one, rb is empty */
auto hasGracePeriod(const ExecutionGraph &g) -> bool;

/** Whether G has a potential race (the cat's pre-race): a plain access and
 * an access of another thread to the same location, which thread creation
 * and join do not order. Without one, the plain-access rules are void */
auto hasPreRace(const ExecutionGraph &g) -> bool;

/** Evaluates the model on G. Unless FULL, only rb and plain-coherence are
 * decided (the other axioms may be wrong in the verdict): events that
 * cannot matter to them are left out */
auto evaluate(const ExecutionGraph &g, bool full) -> Verdict;

/** A data race of G (the cat's data-race flag), if any. G should be
 * consistent */
auto findRace(const ExecutionGraph &g) -> std::optional<Race>;

/** Whether A is before B in (po | rf | tc | tj | rcu-fence)+, GenMC's
 * happens-before with RCU's ordering. A critical section still open counts
 * as closed at the end of its thread: an access is checked when it is added,
 * before its rcu_read_unlock() */
auto isRcuOrderedBefore(const ExecutionGraph &g, const EventLabel *a, const EventLabel *b) -> bool;

} // namespace lkmm

#endif /* GENMC_LKMM_MODEL_HPP */
