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

#ifndef GENMC_MEMORY_MODEL_HPP
#define GENMC_MEMORY_MODEL_HPP

#include "genmc/Support/Error.hpp"

#include <cstdint>
#include <format>
#include <string>

enum class ModelType : std::uint8_t { SC = 0, TSO = 1, RA = 2, RC11 = 3, IMM = 4, LKMM = 5 };

/** Whether, under MODEL, putting an RMW's write part right after the write
 * its read part reads (not necessarily the last write of the location) can
 * make a consistent graph inconsistent, so that the graph must be checked
 * again once the write is placed (ExplorationCore::isPlacementConsistent()).
 * proofs/alloy/rmw-placement.als finds a counterexample under TSO and none
 * under SC, RA and RC11 (scripts/check-alloy-proofs.sh, ctest formal-models,
 * checks this table against it); IMM and LKMM are not modelled, and are
 * checked (LKMM: a cycle through prop is known) */
constexpr auto rmwPlacementNeedsCheck(ModelType model) -> bool
{
	switch (model) {
	case ModelType::SC:
		return false;
	case ModelType::TSO:
		return true;
	case ModelType::RA:
		return false;
	case ModelType::RC11:
		return false;
	case ModelType::IMM:
		return true;
	case ModelType::LKMM:
		return true;
	}
	return true;
}

/** Make `ModelType` formattable with `std::format`. */
template <> struct std::formatter<ModelType> {
	constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

	auto format(const ModelType &model, std::format_context &ctx) const
	{
		switch (model) {
		case ModelType::SC:
			return std::format_to(ctx.out(), "SC");
		case ModelType::TSO:
			return std::format_to(ctx.out(), "TSO");
		case ModelType::RA:
			return std::format_to(ctx.out(), "RA");
		case ModelType::RC11:
			return std::format_to(ctx.out(), "RC11");
		case ModelType::IMM:
			return std::format_to(ctx.out(), "IMM");
		case ModelType::LKMM:
			return std::format_to(ctx.out(), "LKMM");
		default:
			PRINT_BUGREPORT_INFO_ONCE("missing-model-name",
						  "Unknown memory model name");
			return std::format_to(ctx.out(), "UNKNOWN");
		}
	}
};

inline auto isStrongerThan(ModelType model, ModelType other) -> bool
{
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
	/* LKMM is incomparable with the C11 models; SC and TSO are stronger
	 * (x86 implements the kernel's primitives under TSO) */
	static const bool lookup[6][6] = {
		//          SC     TSO    RA     RC11   IMM    LKMM
		/* SC   */ {false, true, true, true, true, true},
		/* TSO  */ {false, false, true, true, true, true},
		/* RA   */ {false, false, false, true, true, false},
		/* RC11 */ {false, false, false, false, true, false},
		/* IMM  */ {false, false, false, false, false, false},
		/* LKMM */ {false, false, false, false, false, false},
	};
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
	return lookup[static_cast<size_t>(model)][static_cast<size_t>(other)];
}

/** Whether the model orders events by syntactic dependencies (addr/data/ctrl) */
inline auto isDepTrackingModel(ModelType model) -> bool
{
	return model == ModelType::IMM || model == ModelType::LKMM;
}

#endif /* GENMC_MEMORY_MODEL_HPP */
