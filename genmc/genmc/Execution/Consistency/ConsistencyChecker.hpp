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

/** The options a memory model reads (from the user's Config). The field
 * names are those of Config, which the generated checkers read through
 * getConf() */
struct ModelOptions {
	bool collectLinSpec = false; /**< Relinche collects a specification */
	bool checkLinSpec = false;   /**< Relinche checks against a specification */
	bool exhaustive = false;     /**< -exhaustive */
	bool lkmmReference = false;  /**< -lkmm-reference */
};

/** A memory model: everything a model offers (see ModelInterfaces.hpp),
 * implemented by a checker generated for each model.
 * This class is not thread-safe: each thread needs its own checker.
 */
class ConsistencyChecker : public GraphModel,
			   public ModelConsistency,
			   public ModelCoherence,
			   public ModelErrors {

protected:
	/* (CONF may be null: default options) */
	ConsistencyChecker(const Config *conf);

public:
	ConsistencyChecker() = delete;
	~ConsistencyChecker() override = default;
	ConsistencyChecker(const ConsistencyChecker &) = delete;
	ConsistencyChecker(ConsistencyChecker &&) = delete;
	auto operator=(const ConsistencyChecker &) -> ConsistencyChecker & = delete;
	auto operator=(ConsistencyChecker &&) -> ConsistencyChecker & = delete;

	static auto create(const Config *conf) -> std::unique_ptr<ConsistencyChecker>;

	/** Returns the options the model reads */
	[[nodiscard]] auto getConf() const -> const ModelOptions * { return &opts_; }

private:
	ModelOptions opts_;
};

#endif /* GENMC_CONSISTENCY_CHECKER_HPP */
