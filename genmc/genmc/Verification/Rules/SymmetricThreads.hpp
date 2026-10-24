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


#ifndef GENMC_SYMMETRIC_THREADS_HPP
#define GENMC_SYMMETRIC_THREADS_HPP

/*
 * Symmetry reduction, at thread creation: which earlier thread a new one is
 * symmetric to (same parent, same function, same argument if it is read,
 * and no access of the parent between the two creations other than to
 * memory only the parent can reach), unless the new thread may read its
 * own id. Pure function of the graph.
 */

class ExecutionGraph;
class ThreadCreateLabel;
struct ThreadInfo;

/** The thread CHILDINFO, created at TCLAB, is symmetric to: the one the user
 * named, or the latest earlier one that qualifies (not if it may read its
 * own thread id); -1 if none */
auto symmetricPredecessor(const ExecutionGraph &g, const ThreadCreateLabel *tcLab,
			  const ThreadInfo &childInfo) -> int;

#endif /* GENMC_SYMMETRIC_THREADS_HPP */
