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

#ifndef GENMC_GRAPH_HASH_HPP
#define GENMC_GRAPH_HASH_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

class ExecutionGraph;

/** A 128-bit hash of a canonical description of an execution graph
 * (see canonicalGraphDescription()). Used by -exhaustive to recognize a graph
 * it has produced before, whichever path produced it. */
struct GraphHash {
	std::uint64_t lo{};
	std::uint64_t hi{};

	auto operator==(const GraphHash &) const -> bool = default;
};

template <> struct std::hash<GraphHash> {
	auto operator()(const GraphHash &h) const noexcept -> std::size_t
	{
		return static_cast<std::size_t>(h.lo ^ (h.hi * 0x9e3779b97f4a7c15ULL));
	}
};

/** A description of G that is the same for two graphs with the same events,
 * rf, co and thread structure, whatever their thread ids and insertion order.
 *
 * Threads are named by their creation path (the index of the create event in
 * the parent, recursively), not by their id: ids are assigned in creation
 * order during a run, so two runs that reach the same graph through different
 * schedules may number nested threads differently. The description lists the
 * threads in the order of their paths; for each, its labels in po, with their
 * kind, ordering, location, size, value, rf (as thread path and index),
 * position in the coherence order of the location, the paths of created and
 * joined threads, and the flags that decide how the run continues (an
 * optional's expansion, a weak CAS's spurious failure). Dynamic addresses,
 * which embed the allocating thread's id, are rewritten the same way.
 * Thread begins whose create is not in the graph (kept as terminated slots
 * after a restriction, see ExecutionGraph::copyGraphUpTo()) are left out. */
auto canonicalGraphDescription(const ExecutionGraph &g) -> std::string;

/** The hash of canonicalGraphDescription(g) */
auto canonicalGraphHash(const ExecutionGraph &g) -> GraphHash;

/** MurmurHash3 (x64, 128-bit) of a byte sequence */
auto murmurHash128(const void *data, std::size_t len, std::uint64_t seed = 0) -> GraphHash;

#endif /* GENMC_GRAPH_HASH_HPP */
