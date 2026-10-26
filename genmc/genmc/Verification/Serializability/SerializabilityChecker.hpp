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

#ifndef GENMC_SERIALIZABILITY_CHECKER_HPP
#define GENMC_SERIALIZABILITY_CHECKER_HPP

#include "genmc/Support/Event.hpp"

#include <optional>
#include <string>

class ExecutionGraph;

/** A cycle among atomic regions (see SerializabilityChecker) */
struct SerializabilityError {
	Event pos;	 /**< An event of the first region on the cycle */
	std::string msg; /**< The regions and the cycle, for the user */
};

/**
 * Checks that an execution graph is conflict-serializable with respect to the
 * atomic regions the program marks (__VERIFIER_serial_begin()/_end(), or any
 * __VERIFIER_method_begin()/_end() pair).
 *
 * The events of each region are contracted to one node, and every event
 * outside a region is a node of its own. The edges are po, rf, co and fr
 * (plus thread create and join) between different nodes. The graph is
 * serializable iff no cycle of the contracted graph goes through a region:
 * the regions then run in some serial order consistent with every conflict.
 * (Under a weak model, a cycle through a region can also be one the model
 * allows without regions: it is reported too, as regions are checked
 * against SC.)
 */
class SerializabilityChecker {
public:
	/** Returns the cycle through a region, if there is one */
	static auto check(const ExecutionGraph &g) -> std::optional<SerializabilityError>;
};

#endif /* GENMC_SERIALIZABILITY_CHECKER_HPP */
