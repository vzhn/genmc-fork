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
 * VSet (ADT/VSet.hpp, a sorted vector: the dependency and event sets)
 * against std::set: its constructors, the bulk insertion and deletion
 * (merges in place), and the set relations.
 */

#include "genmc/ADT/VSet.hpp"

#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <format>
#include <iterator>
#include <set>
#include <vector>

namespace {

using Ref = std::set<int>;

auto elems(const VSet<int> &s) -> std::vector<int> { return {s.begin(), s.end()}; }
auto elems(const Ref &r) -> std::vector<int> { return {r.begin(), r.end()}; }

/* Small values, so that the sets overlap */
auto genVals() -> rc::Gen<std::vector<int>>
{
	return rc::gen::container<std::vector<int>>(rc::gen::inRange(-20, 20));
}

} // namespace

RC_GTEST_PROP(VSet, ConstructorsSortAndDropDuplicates, ())
{
	auto vals = *genVals();
	Ref r(vals.begin(), vals.end());
	RC_ASSERT(elems(VSet<int>(vals.begin(), vals.end())) == elems(r));
	RC_ASSERT(elems(VSet<int>(vals)) == elems(r));
	VSet<int> inserted;
	inserted.insert(vals.begin(), vals.end());
	RC_ASSERT(elems(inserted) == elems(r));
}

RC_GTEST_PROP(VSet, SingleUpdatesAndQueries, ())
{
	VSet<int> s;
	Ref r;
	for (auto v : *genVals()) {
		auto add = *rc::gen::arbitrary<bool>();
		if (add) {
			auto [it, fresh] = s.insert(v);
			RC_ASSERT(fresh == r.insert(v).second);
			RC_ASSERT(*it == v);
		} else {
			RC_ASSERT(s.erase(v) == static_cast<int>(r.erase(v)));
		}
		RC_ASSERT(elems(s) == elems(r));
		RC_ASSERT(s.size() == r.size());
		RC_ASSERT(s.empty() == r.empty());
		for (auto q = -21; q <= 21; q++) {
			RC_ASSERT(s.contains(q) == r.contains(q));
			RC_ASSERT(s.count(q) == static_cast<int>(r.count(q)));
			RC_ASSERT((s.find(q) != s.end()) == r.contains(q));
		}
		if (!r.empty()) {
			RC_ASSERT(s.min() == *r.begin());
			RC_ASSERT(s.max() == *r.rbegin());
		}
	}
}

RC_GTEST_PROP(VSet, BulkUpdatesAndRelations, ())
{
	auto a = *genVals();
	auto b = *genVals();
	VSet<int> sa(a), sb(b);
	Ref ra(a.begin(), a.end()), rb(b.begin(), b.end());

	Ref inter, diff, uni;
	std::ranges::set_intersection(ra, rb, std::inserter(inter, inter.end()));
	std::ranges::set_difference(ra, rb, std::inserter(diff, diff.end()));
	std::ranges::set_union(ra, rb, std::inserter(uni, uni.end()));

	RC_ASSERT(elems(sa.intersectWith(sb)) == elems(inter));
	RC_ASSERT(elems(sa.diff(sb)) == elems(diff));
	RC_ASSERT(sa.intersects(sb) == !inter.empty());
	RC_ASSERT(sa.subsetOf(sb) == std::ranges::includes(rb, ra));
	RC_ASSERT(sa.subsetOf(sa));

	auto merged = sa;
	RC_ASSERT(merged.insert(sb) == static_cast<int>(uni.size() - ra.size()));
	RC_ASSERT(elems(merged) == elems(uni));

	auto erased = sa;
	RC_ASSERT(erased.erase(sb) == static_cast<int>(inter.size()));
	RC_ASSERT(elems(erased) == elems(diff));

	/* (ordered as their elements, lexicographically) */
	RC_ASSERT((sa < sb) == (elems(ra) < elems(rb)));
	RC_ASSERT((sa == sb) == (ra == rb));
}

TEST(VSet, Prints)
{
	EXPECT_EQ(std::format("{}", VSet<int>{3, 1, 2, 1}), "[ 1 2 3 ]");
	EXPECT_EQ(std::format("{}", VSet<int>{}), "[ ]");
}
