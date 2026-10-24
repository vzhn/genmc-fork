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


/*
 * Config::validate(): the options that conflict are rejected, and the ones
 * that have no effect together are switched off with a warning.
 */

#include "genmc/Execution/MemoryModel.hpp"
#include "genmc/Models/BoundDecider.hpp"
#include "genmc/Verification/Config.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <format>

#include <string>
#include <vector>

namespace {

/* The options lli starts from for a verification run under RC11 */
auto rc11() -> Config
{
	Config c;
	c.mode = ExplorationMode::verify;
	c.model = ModelType::RC11;
	return c;
}

struct Outcome {
	std::vector<std::string> errors;
	std::vector<std::string> warnings;

	/* Whether one of MESSAGES contains TEXT */
	static auto has(const std::vector<std::string> &messages, std::string_view text) -> bool
	{
		return std::ranges::any_of(messages, [&](auto &m) { return m.find(text) != m.npos; });
	}
	auto error(std::string_view text) const -> bool { return has(errors, text); }
	auto warning(std::string_view text) const -> bool { return has(warnings, text); }
};

auto validate(Config &c) -> Outcome
{
	Outcome o;
	auto status = c.validate(o.warnings);
	if (auto *errors = std::get_if<ConfigErrorList>(&status))
		o.errors = *errors;
	return o;
}

} // namespace

TEST(Config, TheDefaultsAreValid)
{
	auto c = rc11();
	auto o = validate(c);
	EXPECT_TRUE(o.errors.empty());
	EXPECT_TRUE(o.warnings.empty());
	EXPECT_TRUE(c.randomScheduleSeed.has_value()); /* (filled in) */
}

TEST(Config, ExhaustiveTurnsOffTheReductions)
{
	auto c = rc11();
	c.exhaustive = true;
	c.symmetryReduction = c.ipr = c.helper = c.blindRMWReduction = c.finalWrite = true;
	c.estimate = c.instructionCaching = true;
	c.resumePoints = 8;
	EXPECT_TRUE(validate(c).errors.empty());
	EXPECT_FALSE(c.symmetryReduction || c.ipr || c.helper || c.blindRMWReduction ||
		     c.finalWrite || c.estimate || c.instructionCaching);
	EXPECT_TRUE(c.disableBAM);
	EXPECT_EQ(c.resumePoints, 0U);
}

TEST(Config, ExhaustiveNeedsVerification)
{
	auto c = rc11();
	c.exhaustive = true;
	c.mode = ExplorationMode::random;
	c.randomMax = 1;
	EXPECT_TRUE(validate(c).error("-exhaustive requires --mode=verify"));
}

TEST(Config, DisabledFeaturesAreRejected)
{
	auto c = rc11();
	c.LAPOR = c.confirmation = true;
	auto o = validate(c);
	EXPECT_TRUE(o.error("LAPOR is temporarily disabled"));
	EXPECT_TRUE(o.error("Confirmation is temporarily disabled"));
}

TEST(Config, DependencyTrackingModelsDropIPRAndSymmetryReduction)
{
	for (auto model : {ModelType::IMM, ModelType::LKMM}) {
		auto c = rc11();
		c.model = model;
		c.ipr = c.symmetryReduction = true;
		auto o = validate(c);
		EXPECT_TRUE(o.warning("have no effect under")) << std::format("{}", model);
		EXPECT_FALSE(c.ipr || c.symmetryReduction) << std::format("{}", model);
	}
	auto c = rc11();
	c.ipr = c.symmetryReduction = true;
	EXPECT_TRUE(validate(c).warnings.empty());
	EXPECT_TRUE(c.ipr && c.symmetryReduction);
}

TEST(Config, BlindRMWReductionIsForRC11Only)
{
	auto c = rc11();
	c.model = ModelType::SC;
	c.blindRMWReduction = true;
	EXPECT_TRUE(validate(c).warning("--blind-rmw-reduction has no effect under"));
	EXPECT_FALSE(c.blindRMWReduction);
}

TEST(Config, RandomModeNeedsABudgetAndDropsTheHelper)
{
	auto c = rc11();
	c.mode = ExplorationMode::random;
	c.helper = true;
	auto o = validate(c);
	EXPECT_TRUE(o.error("budget must be greater than 0"));
	EXPECT_TRUE(o.warning("Helper mode is not supported in --mode=random"));
	EXPECT_FALSE(c.helper);
}

TEST(Config, AGraphSizeLimitNeedsPlainRandomRuns)
{
	auto c = rc11();
	c.maxGraphSize = 0;
	c.model = ModelType::IMM;
	c.checkLiveness = true;
	c.checkLinSpec = "/nonexistent";
	c.bound = 2;
	auto o = validate(c);
	for (auto *text : {"requires --mode=random", "does not support", "must be greater than 0",
			   "cannot be used with Relinche", "cannot be used with liveness",
			   "cannot be used with bounding"})
		EXPECT_TRUE(o.error(text)) << text;

	auto r = rc11();
	r.mode = ExplorationMode::random;
	r.randomMax = 10;
	r.maxGraphSize = 100;
	r.symmetryReduction = true;
	auto ro = validate(r);
	EXPECT_TRUE(ro.errors.empty());
	EXPECT_TRUE(ro.warning("Symmetry reduction has no effect with --max-graph-size"));
	EXPECT_FALSE(r.symmetryReduction);
}

TEST(Config, AScheduleSeedNeedsARandomPolicy)
{
	auto c = rc11();
	c.printRandomScheduleSeed = true;
	c.randomScheduleSeed = 42;
	auto o = validate(c);
	EXPECT_TRUE(o.warning("--print-schedule-seed used without"));
	EXPECT_TRUE(o.warning("--schedule-seed used without"));
	EXPECT_EQ(c.randomScheduleSeed, 42U);

	for (auto policy : {SchedulePolicy::Arbitrary, SchedulePolicy::WFR}) {
		auto r = rc11();
		r.schedulePolicy = policy;
		r.printRandomScheduleSeed = true;
		EXPECT_TRUE(validate(r).warnings.empty());
	}
}

TEST(Config, BoundingIsForSCAndTurnsOffTheReductions)
{
	auto c = rc11();
	c.bound = 1;
	EXPECT_TRUE(validate(c).error("Bounding can only be used with --sc"));

	auto s = rc11();
	s.model = ModelType::SC;
	s.bound = 1;
	s.symmetryReduction = s.ipr = true;
	s.schedulePolicy = SchedulePolicy::WF;
	auto o = validate(s);
	EXPECT_TRUE(o.errors.empty());
	EXPECT_TRUE(o.warning("have no effect when --bound is used"));
	EXPECT_FALSE(s.symmetryReduction || s.ipr);
	EXPECT_TRUE(s.disableBAM);
	EXPECT_EQ(s.schedulePolicy, SchedulePolicy::LTR);

	auto t = rc11();
	t.boundType = BoundType::round;
	EXPECT_TRUE(validate(t).warning("--bound-type used without --bound"));
}

TEST(Config, RelincheCollectsOrChecksAnExistingSpecification)
{
	auto c = rc11();
	c.collectLinSpec = "spec.out";
	c.checkLinSpec = "/nonexistent/spec";
	auto o = validate(c);
	EXPECT_TRUE(o.error("Cannot collect and analyze"));
	EXPECT_TRUE(o.error("Specification file is not a regular file"));

	auto d = rc11();
	d.checkLinSpec = "/";
	EXPECT_TRUE(validate(d).error("not a regular file"));
}
