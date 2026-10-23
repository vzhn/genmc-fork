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


#include "genmc/Verification/Rules/KeptEventAlignment.hpp"
#include "genmc/Execution/EventKey.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"

#include <algorithm>
#include <ranges>

namespace kept_events {

auto align(const ExecutionGraph &g, Event curr, const EventKey &key, std::uint32_t sub)
	-> Alignment
{
	using Kind = Alignment::Kind;

	auto isOwn = [&](const EventLabel *oLab) {
		return oLab->getKey() && *oLab->getKey() == key && oLab->getKeySub() == sub;
	};
	/* The thread's events after CURR, holes left out */
	auto size = g.getThreadSize(curr.thread);
	auto later = std::views::iota(std::min(curr.index + 1, size), size) |
		     std::views::transform([&g, curr](int i) {
			     return g.getEventLabel(Event(curr.thread, i));
		     }) |
		     std::views::filter([](auto *oLab) { return !genmc::isa<EmptyLabel>(oLab); });

	const auto *lab = g.getEventLabelIfPresent(curr);
	if (lab != nullptr && !genmc::isa<EmptyLabel>(lab)) {
		if (isOwn(lab))
			return {.kind = Kind::Own};
		if (!lab->getKey())
			return {.kind = Kind::MissingKey};

		/* A new event before kept ones */
		return {.kind = std::ranges::any_of(later, isOwn) ? Kind::ShiftAndDrop
								  : Kind::OpenGap};
	}

	/* A hole: fill it, unless this instance's event is kept further on:
	 * after holes only (the old path was longer), move it here; after other
	 * kept events, those would be skipped */
	auto first = std::ranges::begin(later);
	if (first == std::ranges::end(later))
		return {.kind = Kind::FillHole};
	if (isOwn(*first))
		return {.kind = Kind::MoveHere, .from = (*first)->getIndex()};
	return {.kind = std::ranges::any_of(later, isOwn) ? Kind::Drop : Kind::FillBeforeKept};
}

} // namespace kept_events
