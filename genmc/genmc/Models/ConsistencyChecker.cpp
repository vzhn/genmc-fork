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

#include "genmc/Models/ConsistencyChecker.hpp"
#include "genmc/Models/IMMChecker.hpp"  // NOLINT(misc-include-cleaner)
#include "genmc/Models/LKMMRcuChecker.hpp" // NOLINT(misc-include-cleaner)
#include "genmc/Models/RAChecker.hpp"   // NOLINT(misc-include-cleaner)
#include "genmc/Models/RC11Checker.hpp" // NOLINT(misc-include-cleaner)
#include "genmc/Models/SCChecker.hpp"   // NOLINT(misc-include-cleaner)
#include "genmc/Models/TSOChecker.hpp"  // NOLINT(misc-include-cleaner)
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/MemoryModel.hpp"
#include "genmc/Support/Error.hpp"

#include <memory>

static auto createChecker(ModelType model) -> std::unique_ptr<ConsistencyChecker>
{
#define CREATE_CHECKER(_model)                                                                     \
	case ModelType::_model:                                                                    \
		return std::make_unique<_model##Checker>(nullptr);

	switch (model) {
		CREATE_CHECKER(SC);
		CREATE_CHECKER(TSO);
		CREATE_CHECKER(RA);
		CREATE_CHECKER(RC11);
		CREATE_CHECKER(IMM);
	case ModelType::LKMM:
		return std::make_unique<LKMMRcuChecker>(nullptr);
	default:
		UNREACHABLE();
	}
}

auto ConsistencyChecker::create(ModelType model, ModelOptions opts)
	-> std::unique_ptr<ConsistencyChecker>
{
	auto checker = createChecker(model);
	checker->opts_ = opts;
	return checker;
}
