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


#include "genmc/Verification/Core/Explorer.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Support/Error.hpp"
#include "genmc/Verification/Core/EventProducer.hpp"
#include "genmc/Verification/Core/GenMCDriver.hpp"

#include <vector>

/* Each thread's last event in G (see EventProducer::resumeFrom()) */
static auto lastIndices(const ExecutionGraph &g) -> std::vector<int>
{
	std::vector<int> v;
	for (auto i = 0U; i < g.getNumThreads(); i++)
		v.push_back(g.getThreadSize(i) - 1);
	return v;
}

/* The threads of G other than main (see EventProducer::runFromStart()) */
static auto threadInfos(const ExecutionGraph &g) -> std::vector<ThreadInfo>
{
	std::vector<ThreadInfo> tis;
	for (auto i = 1U; i < g.getNumThreads(); i++) { // skip main
		const auto *bLab = g.getFirstThreadLabel(i);
		VERIFY(bLab);
		tis.push_back(bLab->getThreadInfo());
	}
	return tis;
}

void explore(GenMCDriver &driver, EventProducer &producer)
{
	driver.setResumeHook([&producer](Event pos) { return producer.saveResumePoint(pos); });
	do {
		auto errorReplay = driver.handleExecutionStart();
		const auto &g = driver.getExec().getGraph();
		if (!errorReplay && driver.runFromCache()) {
			driver.handleExecutionEnd();
			continue;
		}
		auto rp = driver.takePendingResumePoint();
		auto resumed =
			rp && !errorReplay && producer.resumeFrom(*rp, threadInfos(g), lastIndices(g));
		if (!resumed)
			producer.runFromStart(threadInfos(g), errorReplay);
		driver.handleExecutionEnd();
	} while (!driver.done());
	driver.setResumeHook(nullptr);
}
