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


#include "genmc/Verification/State/ExecutionFrame.hpp"

ExecutionFrame::ExecutionFrame(std::unique_ptr<ExecutionGraph> g, WorkList &&w, ChoiceMap &&map)
	: graph(std::move(g)), workqueue(std::move(w)), choices(std::move(map))
{}

ExecutionFrame::~ExecutionFrame() = default;

void ExecutionFrame::restrict(Stamp stamp)
{
	auto &g = getGraph();
	g.cutToStamp(stamp);
	getChoiceMap().cut(*g.getViewFromStamp(stamp));
}
