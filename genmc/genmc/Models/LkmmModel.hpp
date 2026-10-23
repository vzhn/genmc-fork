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

#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

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

/** Whether G has a synchronize_rcu() or synchronize_srcu(): without one, rb
 * is empty */
/** Whether a thread of G stopped before events a revisit kept (its block
 * went after them, see blockThread()) */
auto hasThreadBlockedBeforeKept(const ExecutionGraph &g) -> bool;

auto hasGracePeriod(const ExecutionGraph &g) -> bool;

/** Whether G has a potential race (the cat's pre-race): a plain access and
 * an access of another thread to the same location, which thread creation
 * and join do not order. Without one, the plain-access rules are void */
auto hasPreRace(const ExecutionGraph &g) -> bool;

/** The srcu_read_lock()s that UNLOCK, an srcu_read_unlock() of G, matches:
 * those of its srcu_struct whose value it writes back, by data dependencies
 * (through memory, thread creation and join, but not through another
 * unlock) */
auto srcuLocksOf(const ExecutionGraph &g, const EventLabel *unlock)
	-> std::vector<const EventLabel *>;

/** A synchronize_srcu() of UNLOCK's srcu_struct inside the critical
 * section that UNLOCK closes in its thread, if any: it waits for its own
 * reader, and every execution is inconsistent */
auto srcuOwnGracePeriod(const ExecutionGraph &g, const EventLabel *unlock) -> const EventLabel *;

/** The cat's srcu-rscs on G: each srcu_read_unlock() with the
 * srcu_read_lock()s of its srcu_struct whose value it writes back, by data
 * dependencies (through memory, but not through another unlock) */
auto srcuRscs(const ExecutionGraph &g)
	-> std::vector<std::pair<const EventLabel *, const EventLabel *>>;

/** A misuse of SRCU (the bell's flags): an event and what it concerns */
struct SrcuMisuse {
	const EventLabel *lab = nullptr;
	const EventLabel *other = nullptr;
	std::string what;
};

/** A misuse of SRCU in G, a complete execution, if any: an unmatched
 * srcu_read_lock() or srcu_read_unlock(), a lock with two unlocks, an unlock
 * of another value than its lock read */
auto findSrcuMisuse(const ExecutionGraph &g) -> std::optional<SrcuMisuse>;

/** Evaluates the model on G. Unless FULL, only rb and plain-coherence are
 * decided (the other axioms may be wrong in the verdict): events that
 * cannot matter to them are left out. With CLOSEOPEN, a critical section
 * still open counts as closed at the end of its thread (see Model) */
auto evaluate(const ExecutionGraph &g, bool full, bool closeOpen = false) -> Verdict;

/** A data race of G (the cat's data-race flag), if any. G should be
 * consistent */
auto findRace(const ExecutionGraph &g) -> std::optional<Race>;

/** An access of freed memory in G, with the free, that neither ISHB (GenMC's
 * happens-before) nor (po | rf | tc | tj | rcu-fence)+ orders before the
 * free, every critical section matched (G is complete) */
auto findUnorderedFree(const ExecutionGraph &g,
		       const std::function<bool(const EventLabel *, const EventLabel *)> &isHb)
	-> std::optional<std::pair<const EventLabel *, const EventLabel *>>;

/** Whether A is before B in (po | rf | tc | tj | rcu-fence)+, GenMC's
 * happens-before with RCU's ordering. With CLOSEOPEN, a critical section
 * still open counts as closed at the end of its thread: an access is
 * checked when it is added, before its rcu_read_unlock(). (An SRCU section
 * closed in another thread, by srcu_up_read(), may end earlier: such a
 * graph is checked again when complete, without CLOSEOPEN) */
auto isRcuOrderedBefore(const ExecutionGraph &g, const EventLabel *a, const EventLabel *b,
			bool closeOpen = true) -> bool;

} // namespace lkmm

#endif /* GENMC_LKMM_MODEL_HPP */
