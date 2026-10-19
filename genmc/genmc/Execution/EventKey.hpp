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

#ifndef GENMC_EVENT_KEY_HPP
#define GENMC_EVENT_KEY_HPP

#include <cstdint>
#include <format>
#include <string>
#include <utility>
#include <vector>

/**
 * Identifies the dynamic instance of the instruction that produced an event,
 * independently of the event's position in its thread: for each frame of the
 * thread's stack, the call instruction (or, for the innermost frame, the
 * instruction itself) and how many times it ran before in its frame.
 * An event's key is the key of its instruction instance plus its index among
 * the events that instance produced (see EventLabel::getKeySub()).
 *
 * Instructions are numbered by the interpreter (in module order, so that
 * keys agree between the module copies of parallel workers; 0 stands for
 * no instruction, e.g. a thread's end); only the interpreter creates keys.
 */
class EventKey {
public:
	using Step = std::pair<std::uint32_t, std::uint32_t>;

	EventKey() = default;
	explicit EventKey(std::vector<Step> path) : path(std::move(path)) {}

	auto getPath() const -> const std::vector<Step> & { return path; }

	auto operator==(const EventKey &other) const -> bool = default;

	auto toString() const -> std::string
	{
		std::string s;
		for (const auto &[inst, occ] : path)
			s += std::format("{}{}#{}", s.empty() ? "" : "/", inst, occ);
		return s;
	}

private:
	std::vector<Step> path;
};

#endif /* GENMC_EVENT_KEY_HPP */
