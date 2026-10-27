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

#ifndef GENMC_IMM_FULL_CHECKER_HPP
#define GENMC_IMM_FULL_CHECKER_HPP

#include "genmc/Models/IMMChecker.hpp"

/** IMM. The Kater-generated IMMChecker explores a weaker model, IMM without
 * the po? of its addr ; po? (vzhn/kater-fork#38): with it, a read in a hole
 * orders the event it depends on by address before the writes after it,
 * whatever it reads, and the exploration can keep a graph whose holes no
 * events fill consistently. The weaker model is extensible. IMM's own ar is
 * the generated arFull(), which isConsistent() leaves out; it is checked
 * here of complete executions and of the graphs of errors, after Bouajjani
 * et al. (2023), as Mixer (POPL 2025, Section 2.4) describes */
class IMMFullChecker : public IMMChecker {
public:
	IMMFullChecker(const Config *conf) : IMMChecker(conf) {}

	auto satisfiesExtraAxioms(const ExecutionGraph &g) -> bool override;

private:
	/* IMM's full ar is acyclic in G. The generated arFull(lab) is an
	 * incremental check, of the cycles through an event just added at the
	 * end of its thread: checked of every event of a graph, it misses
	 * cycles. This is the generated whole-graph check of the export
	 * (checkConsAcyclic3() is the acyclicity of arFull in IMMChecker.cpp),
	 * until Kater emits it under the export's name */
	auto arFull(const ExecutionGraph &g) const -> bool { return checkConsAcyclic3(g); }
};

#endif /* GENMC_IMM_FULL_CHECKER_HPP */
