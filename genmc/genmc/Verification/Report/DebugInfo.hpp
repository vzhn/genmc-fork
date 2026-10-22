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


#ifndef GENMC_DEBUG_INFO_HPP
#define GENMC_DEBUG_INFO_HPP

#include "genmc/Support/Event.hpp"

#include <string>
#include <unordered_map>

/** Debug information for an instruction (collected by an error replay) */
struct EventDbgInfo {
	std::string file;
	int line = 0;
	std::string functionName;
	std::string source;
	std::string accessedVarName;
};

/** Packs together debug information for a graph */
using GraphDbgInfo = std::unordered_map<Event, EventDbgInfo>;

#endif /* GENMC_DEBUG_INFO_HPP */
