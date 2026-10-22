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


#include "genmc/Execution/Consistency/ModelInterfaces.hpp"
#include "genmc/ADT/VectorClock.hpp"
#include "genmc/Execution/EventLabel.hpp"

#include <memory>

auto GraphModel::getHbView(const EventLabel *lab) const -> const View &
{
	return lab->view(getHbViewIndex());
}

auto GraphModel::calculateRevisitPrefixView(const EventLabel *lab) const
	-> std::unique_ptr<VectorClock>
{
	return calculatePrefixView(lab);
}
