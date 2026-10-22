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

#ifndef GENMC_CONSISTENCY_CHECKER_HPP
#define GENMC_CONSISTENCY_CHECKER_HPP

#include "genmc/Execution/Consistency/ModelInterfaces.hpp"

#include <cstdint>
#include <memory>
#include <vector>

class EventLabel;
class ReadLabel;
class WriteLabel;
class ExecutionGraph;
struct Config;
class VectorClock;
class View;
enum class ModelType : std::uint8_t;

/** A memory model: everything a model offers (see ModelInterfaces.hpp),
 * implemented by a checker generated for each model.
 * This class is not thread-safe: each thread needs its own checker.
 */
class ConsistencyChecker : public GraphModel,
			   public ModelConsistency,
			   public ModelCoherence,
			   public ModelErrors {

protected:
	ConsistencyChecker(const Config *conf) : conf_(conf) {}

public:
	ConsistencyChecker() = delete;
	~ConsistencyChecker() override = default;
	ConsistencyChecker(const ConsistencyChecker &) = delete;
	ConsistencyChecker(ConsistencyChecker &&) = delete;
	auto operator=(const ConsistencyChecker &) -> ConsistencyChecker & = delete;
	auto operator=(ConsistencyChecker &&) -> ConsistencyChecker & = delete;

	static auto create(const Config *conf) -> std::unique_ptr<ConsistencyChecker>;

	/** Returns the user config */
	[[nodiscard]] auto getConf() const -> const Config * { return conf_; }

private:
	/* Keep the config around for convenience (e.g., for optimizing
	 * consistency checking routines) */
	const Config *conf_;
};

#endif /* GENMC_CONSISTENCY_CHECKER_HPP */
