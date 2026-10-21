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

#ifndef GENMC_LOOP_KEY_HPP
#define GENMC_LOOP_KEY_HPP

#include <cstdint>

/**
 * Identifies the loop that a loop begin or a spin start belongs to, so that
 * the spinloop detection can tell nested loops apart: the number that the
 * spin-assume transformation gave the loop (0 for a manually instrumented
 * one), and the depth of the call stack that runs it (a recursive call may
 * run the same loop inside itself).
 */
struct LoopKey {
	std::uint32_t id = 0;
	std::uint32_t depth = 0;

	auto operator==(const LoopKey &other) const -> bool = default;
};

#endif /* GENMC_LOOP_KEY_HPP */
