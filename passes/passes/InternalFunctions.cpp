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

#include "passes/InternalFunctions.hpp"
#include <string>
#include <unordered_map>

const std::unordered_map<std::string, InternalFunctions> internalFunNames = {
#define HANDLE_FUNCTION(NUM, FUN, NAME) {FUN, InternalFunctions::NAME},
#include "passes/InternalFunction.def"

	/* Some extra C++ calls */
	{"_Znwm", InternalFunctions::Malloc},
	{"_ZdlPv", InternalFunctions::Free},

	/* SV-COMP's other integer __VERIFIER_nondet_X(): the value is fitted
	 * to the return type, so the types are not needed here */
	{"__VERIFIER_nondet_bool", InternalFunctions::NondetInt},
	{"__VERIFIER_nondet_char", InternalFunctions::NondetInt},
	{"__VERIFIER_nondet_uchar", InternalFunctions::NondetInt},
	{"__VERIFIER_nondet_short", InternalFunctions::NondetInt},
	{"__VERIFIER_nondet_ushort", InternalFunctions::NondetInt},
	{"__VERIFIER_nondet_uint", InternalFunctions::NondetInt},
	{"__VERIFIER_nondet_unsigned", InternalFunctions::NondetInt},
	{"__VERIFIER_nondet_long", InternalFunctions::NondetInt},
	{"__VERIFIER_nondet_ulong", InternalFunctions::NondetInt},
	{"__VERIFIER_nondet_longlong", InternalFunctions::NondetInt},
	{"__VERIFIER_nondet_ulonglong", InternalFunctions::NondetInt},
	{"__VERIFIER_nondet_int128", InternalFunctions::NondetInt},
	{"__VERIFIER_nondet_uint128", InternalFunctions::NondetInt},
	{"__VERIFIER_nondet_size_t", InternalFunctions::NondetInt},
	{"__VERIFIER_nondet_u32", InternalFunctions::NondetInt},
	{"__VERIFIER_nondet_loff_t", InternalFunctions::NondetInt},
	{"__VERIFIER_nondet_sector_t", InternalFunctions::NondetInt},
};
