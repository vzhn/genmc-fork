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

#ifndef GENMC_GRAPH_FINGERPRINT_HPP
#define GENMC_GRAPH_FINGERPRINT_HPP

#include "genmc/Execution/Event.hpp"
#include "genmc/Execution/Stamp.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

/**
 * A summary of an execution graph, taken when the runtime's state is saved
 * for the forward revisits of an event (see ResumePoint) and compared with
 * the graph a resumed execution starts from (-check-invariants, I4 in
 * Invariants.hpp). It is header-light on purpose: ResumePoint.hpp carries
 * it, and GenMCDriver.hpp includes that.
 */
struct GraphFingerprint {
	/** The event the state was saved at (its rf/co is what the revisits change) */
	Event saved;
	/** The graph's maximum stamp at the time (for the report only) */
	Stamp maxStamp{0};
	/** Per thread: the index of its last label (holes included) */
	std::vector<int> lastIndex;
	/** Per thread: a hash of its labels (kinds, positions, addresses, sizes,
	 * values, rf sources, create/join links; SAVED's own rf excluded) */
	std::vector<std::size_t> threadHash;
	/** Per location with writes (in address order): the address and a hash
	 * of its coherence order, SAVED excluded */
	std::vector<std::pair<std::uint64_t, std::size_t>> coHashes;
};

#endif /* GENMC_GRAPH_FINGERPRINT_HPP */
