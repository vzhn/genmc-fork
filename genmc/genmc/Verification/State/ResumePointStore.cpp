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


#include "genmc/Verification/State/ResumePointStore.hpp"

#include <algorithm>

auto ResumePointStore::save(Event pos) -> std::shared_ptr<ResumePoint>
{
	auto rp = hook_ ? hook_(pos) : nullptr;
	if (!rp)
		return nullptr;
	saved_.push_back(rp);
	if (saved_.size() > capacity_)
		saved_.pop_front();
	return rp;
}

void ResumePointStore::resumeAt(std::shared_ptr<ResumePoint> rp)
{
	pending_ = std::move(rp);
	if (pending_ && --pending_->pending == 0)
		std::erase(saved_, pending_); /* its last revisit */
}
