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


#ifndef GENMC_RESUME_POINT_STORE_HPP
#define GENMC_RESUME_POINT_STORE_HPP

#include "genmc/Support/Event.hpp"
#include "genmc/Verification/ResumePoint.hpp"

#include <deque>
#include <functional>
#include <memory>

/**
 * The runtime states saved for forward revisits (see ResumePoint): the
 * producer's hook that saves one, the newest CAPACITY saved states, and
 * the one the next execution resumes from. A state is dropped when its
 * last revisit is taken or when newer ones push it out; a revisit whose
 * state was dropped replays the graph instead.
 */
class ResumePointStore {
public:
	using Hook = std::function<std::shared_ptr<ResumePoint>(Event)>;

	explicit ResumePointStore(unsigned capacity) : capacity_(capacity) {}

	/** HOOK saves the producer's state at an event, or returns null */
	void setHook(Hook hook) { hook_ = std::move(hook); }
	[[nodiscard]] auto hasHook() const -> bool { return static_cast<bool>(hook_); }

	/** Saves the producer's state at POS and keeps it among the newest
	 * (null if the producer cannot) */
	auto save(Event pos) -> std::shared_ptr<ResumePoint>;

	/** A forward revisit restored the graph to the time RP was saved: the
	 * next execution resumes from RP (none if null). One fewer revisit
	 * refers to RP; it is dropped after its last */
	void resumeAt(std::shared_ptr<ResumePoint> rp);

	/** The state the next execution resumes from, if any */
	[[nodiscard]] auto pending() const -> const std::shared_ptr<ResumePoint> & { return pending_; }

	/** Takes the pending state (the next execution resumes from it) */
	auto takePending() -> std::shared_ptr<ResumePoint> { return std::exchange(pending_, nullptr); }

	/** The next execution starts over (it replays the graph) */
	void clearPending() { pending_ = nullptr; }

	/** The number of states kept */
	[[nodiscard]] auto size() const -> size_t { return saved_.size(); }

private:
	unsigned capacity_;
	Hook hook_;
	std::deque<std::shared_ptr<ResumePoint>> saved_;
	std::shared_ptr<ResumePoint> pending_;
};

#endif /* GENMC_RESUME_POINT_STORE_HPP */
