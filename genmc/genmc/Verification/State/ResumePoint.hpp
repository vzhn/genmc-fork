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

#ifndef GENMC_RESUME_POINT_HPP
#define GENMC_RESUME_POINT_HPP

#include "genmc/Support/Event.hpp"
#include "genmc/Verification/Checks/GraphFingerprint.hpp"

#include <optional>
#include <vector>

/**
 * The runtime's state at an event, saved when the event's forward revisits
 * are queued. A forward revisit restricts the graph to what existed when the
 * event was added, so the execution exploring it can resume from this state
 * instead of replaying the whole graph through the runtime. The runtime
 * defines the concrete type (see Interpreter::saveResumePoint()).
 */
class ResumePoint {
public:
	ResumePoint() = default;
	ResumePoint(const ResumePoint &) = delete;
	auto operator=(const ResumePoint &) -> ResumePoint & = delete;
	virtual ~ResumePoint() = default;

	/** How many queued revisits still refer to this state (the driver
	 * drops a state nothing refers to, see GenMCDriver::saveResumePoint()) */
	unsigned pending = 0;

	/** With -check-invariants: the graph at the time of the save, to compare
	 * with the graph a revisit resumes from (see Invariants.hpp, I4) */
	std::optional<GraphFingerprint> fingerprint;

	/** Each thread's position (its last event) in the saved state, for the
	 * same check; empty if the runtime does not tell */
	[[nodiscard]] virtual auto threadPositions() const -> std::vector<Event> { return {}; }
};

#endif /* GENMC_RESUME_POINT_HPP */
