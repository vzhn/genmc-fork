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
 * AdaptiveView (ADT/AdaptiveView.hpp, the last accesses to a location
 * that race detection checks): a view stored as one event, a few events or
 * a full View, promoted as it grows. Joins agree with View's, whatever the
 * representations; an update by an event keeps it and every event its view
 * does not contain, and adds nothing else.
 */

#include "genmc/ADT/AdaptiveView.hpp"
#include "genmc/ADT/View.hpp"

#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <vector>

namespace {

/* More threads than the inline capacity (8), so that Events get promoted */
constexpr auto kThreads = 12;

auto genEvent() -> rc::Gen<Event>
{
	return rc::gen::apply([](int t, int i) { return Event(t, i); },
			      rc::gen::inRange(0, kThreads), rc::gen::inRange(1, 10));
}

auto viewOf(const std::vector<Event> &es) -> View
{
	View v;
	for (const auto &e : es)
		v.updateIdx(e);
	return v;
}

/* An operand in one of the representations, and the view it stands for */
struct Operand {
	AdaptiveView av;
	View view;
};

auto genOperand() -> rc::Gen<Operand>
{
	return rc::gen::mapcat(rc::gen::inRange(0, 5), [](int kind) -> rc::Gen<Operand> {
		switch (kind) {
		case 0:
			return rc::gen::just(Operand{AdaptiveView(), View()});
		case 1:
			return rc::gen::map(genEvent(), [](Event e) {
				return Operand{AdaptiveView(e), viewOf({e})};
			});
		case 2: /* events of distinct threads, at most the inline capacity */
			return rc::gen::map(
				rc::gen::container<std::vector<Event>>(genEvent()),
				[](std::vector<Event> es) {
					AdaptiveView::Events small;
					std::vector<Event> kept;
					for (const auto &e : es) {
						auto dup = std::ranges::any_of(kept, [&](auto &k) {
							return k.thread == e.thread;
						});
						if (!dup && kept.size() < AdaptiveView::SmallVectorCapacity)
							kept.push_back(e);
					}
					for (const auto &e : kept)
						small.push_back(e);
					return Operand{AdaptiveView(small), viewOf(kept)};
				});
		case 3:
			return rc::gen::map(rc::gen::container<std::vector<Event>>(genEvent()),
					    [](std::vector<Event> es) {
						    auto v = viewOf(es);
						    return Operand{AdaptiveView(v), v};
					    });
		default: /* an update: stands for its event alone */
			return rc::gen::apply(
				[](Event e, std::vector<Event> es) {
					return Operand{AdaptiveView(e, viewOf(es)), viewOf({e})};
				},
				genEvent(), rc::gen::container<std::vector<Event>>(genEvent()));
		}
	});
}

/* The view an adaptive view's events stand for */
auto asView(const AdaptiveView &av) -> View
{
	View v;
	for (auto e : av)
		if (e.index >= 0)
			v.updateIdx(e);
	return v;
}

auto sameView(const View &a, const View &b) -> bool
{
	for (auto t = 0; t <= kThreads; t++)
		if (a.getMax(t) != b.getMax(t))
			return false;
	return true;
}

auto contains(const View &outer, const View &inner) -> bool
{
	for (auto t = 0; t <= kThreads; t++)
		if (inner.getMax(t) > outer.getMax(t))
			return false;
	return true;
}

/* Whether some event of AV on E's thread is at E or after it */
auto covers(const AdaptiveView &av, Event e) -> bool
{
	return std::ranges::any_of(av, [&](Event o) { return o.thread == e.thread && o.index >= e.index; });
}

} // namespace

RC_GTEST_PROP(AdaptiveView, JoinsAgreeWithViews, ())
{
	auto acc = *genOperand();
	auto ref = acc.view;
	RC_ASSERT(sameView(asView(acc.av), ref));
	for (const auto &op : *rc::gen::container<std::vector<Operand>>(genOperand())) {
		acc.av.join(op.av);
		ref.update(op.view);
		RC_ASSERT(sameView(asView(acc.av), ref));

		auto probe = viewOf(*rc::gen::container<std::vector<Event>>(genEvent()));
		RC_ASSERT(acc.av.containedIn(probe) == contains(probe, ref));
		RC_ASSERT(acc.av.containedIn(ref));
	}
	auto self = acc.av;
	self.join(self);
	RC_ASSERT(sameView(asView(self), ref));
}

RC_GTEST_PROP(AdaptiveView, AnUpdateKeepsWhatItsViewDoesNotContain, ())
{
	auto acc = *genOperand();
	/* (an update's left-hand side is not itself an update with a view) */
	if (acc.av.size() == 1)
		acc.av = AdaptiveView(*acc.av.begin());
	for (const auto &op : *rc::gen::container<std::vector<Operand>>(genOperand()))
		acc.av.join(op.av);
	std::vector<Event> before(acc.av.begin(), acc.av.end());

	auto pos = *genEvent();
	auto uView = viewOf(*rc::gen::container<std::vector<Event>>(genEvent()));
	acc.av += AdaptiveView(pos, uView);

	RC_ASSERT(covers(acc.av, pos));
	for (const auto &e : before)
		if (e.index >= 0 && !uView.contains(e))
			RC_ASSERT(covers(acc.av, e));
	for (auto e : acc.av) {
		if (e.index < 0)
			continue;
		RC_ASSERT(e == pos || std::ranges::find(before, e) != before.end());
	}
	acc.av += AdaptiveView(); /* (an empty update changes nothing) */
	RC_ASSERT(covers(acc.av, pos));
}

TEST(AdaptiveView, GrowsFromAnEventToAView)
{
	AdaptiveView av(Event(0, 1));
	EXPECT_EQ(av.size(), 1U);
	av.join(Event(0, 3));
	EXPECT_EQ(av.size(), 1U);
	EXPECT_EQ(*av.begin(), Event(0, 3));
	for (auto t = 1; t < 9; t++)
		av.join(Event(t, t));
	/* Nine threads: promoted to a View, one entry per thread */
	EXPECT_EQ(av.size(), 9U);
	EXPECT_TRUE(av.containedIn(viewOf({Event(0, 3), Event(1, 1), Event(2, 2), Event(3, 3),
					   Event(4, 4), Event(5, 5), Event(6, 6), Event(7, 7),
					   Event(8, 8)})));
	EXPECT_FALSE(av.containedIn(viewOf({Event(0, 3)})));
	EXPECT_TRUE(AdaptiveView().empty());
	EXPECT_TRUE(AdaptiveView().containedIn(View()));
}

TEST(AdaptiveView, EqualityComparesSingleEventsAcrossRepresentations)
{
	AdaptiveView::Events one;
	one.push_back(Event(2, 4));
	EXPECT_EQ(AdaptiveView(Event(2, 4)), AdaptiveView(one));
	EXPECT_EQ(AdaptiveView(Event(2, 4)), AdaptiveView(Event(2, 4), View()));
	EXPECT_NE(AdaptiveView(Event(2, 4)), AdaptiveView(Event(2, 5)));
	EXPECT_EQ(AdaptiveView(), AdaptiveView());
}
