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
 * genmc::BitVector (ADT/BitVector.hpp, used by Relinche) against a
 * std::vector<bool>: random sequences of updates across word boundaries,
 * after each of which every query agrees.
 */

#include "genmc/ADT/BitVector.hpp"

#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <cstdint>
#include <vector>

namespace {

using Ref = std::vector<bool>;

enum class Kind : std::uint8_t {
	Set,
	Reset,
	Flip,
	SetAll,
	ResetAll,
	FlipAll,
	SetRange,
	ResetRange,
	Resize,
	PushBack,
	PopBack,
	Shl,
	Shr,
	Or,
	And,
	Xor,
	ResetMask,
	Swap,
	Last = Swap
};

struct Op {
	Kind kind;
	unsigned a; /* an index, size or shift */
	unsigned b; /* a range's other end */
	bool bit;
	std::vector<bool> other; /* the right-hand side of a bitwise operation */
};

auto make(const Ref &r) -> genmc::BitVector
{
	genmc::BitVector v(r.size());
	for (auto i = 0U; i < r.size(); i++)
		if (r[i])
			v.set(i);
	return v;
}

/* Applies OP to V and to its reference R (indices are taken modulo the size) */
void apply(const Op &op, genmc::BitVector &v, Ref &r)
{
	auto n = static_cast<unsigned>(r.size());
	auto idx = n ? op.a % n : 0;
	auto lo = n ? std::min(op.a % (n + 1), op.b % (n + 1)) : 0;
	auto hi = n ? std::max(op.a % (n + 1), op.b % (n + 1)) : 0;
	switch (op.kind) {
	case Kind::Set:
		if (n) {
			v.set(idx);
			r[idx] = true;
		}
		break;
	case Kind::Reset:
		if (n) {
			v.reset(idx);
			r[idx] = false;
		}
		break;
	case Kind::Flip:
		if (n) {
			v.flip(idx);
			r[idx] = !r[idx];
		}
		break;
	case Kind::SetAll:
		v.set();
		std::fill(r.begin(), r.end(), true);
		break;
	case Kind::ResetAll:
		v.reset();
		std::fill(r.begin(), r.end(), false);
		break;
	case Kind::FlipAll:
		v.flip();
		r.flip();
		break;
	case Kind::SetRange:
		v.set(lo, hi);
		std::fill(r.begin() + lo, r.begin() + hi, true);
		break;
	case Kind::ResetRange:
		v.reset(lo, hi);
		std::fill(r.begin() + lo, r.begin() + hi, false);
		break;
	case Kind::Resize:
		v.resize(op.a % 200, op.bit);
		r.resize(op.a % 200, op.bit);
		break;
	case Kind::PushBack:
		v.push_back(op.bit);
		r.push_back(op.bit);
		break;
	case Kind::PopBack:
		if (n) {
			v.pop_back();
			r.pop_back();
		}
		break;
	case Kind::Shl: {
		auto s = n ? op.a % (n + 1) : 0;
		v <<= s;
		Ref out(n, false);
		for (auto i = s; i < n; i++)
			out[i] = r[i - s];
		r = out;
		break;
	}
	case Kind::Shr: {
		auto s = n ? op.a % (n + 1) : 0;
		v >>= s;
		Ref out(n, false);
		for (auto i = 0U; i + s < n; i++)
			out[i] = r[i + s];
		r = out;
		break;
	}
	case Kind::Or:
	case Kind::And:
	case Kind::Xor:
	case Kind::ResetMask: {
		/* (the same size: &= and reset() take a shorter one as zeros, |=
		 * and ^= grow to a longer one) */
		Ref o(op.other.begin(), op.other.end());
		o.resize(n, false);
		auto ov = make(o);
		for (auto i = 0U; i < n; i++) {
			switch (op.kind) {
			case Kind::Or:
				r[i] = r[i] || o[i];
				break;
			case Kind::And:
				r[i] = r[i] && o[i];
				break;
			case Kind::Xor:
				r[i] = r[i] != o[i];
				break;
			default:
				r[i] = r[i] && !o[i];
				break;
			}
		}
		if (op.kind == Kind::Or)
			v |= ov;
		else if (op.kind == Kind::And)
			v &= ov;
		else if (op.kind == Kind::Xor)
			v ^= ov;
		else
			v.reset(ov);
		break;
	}
	case Kind::Swap: {
		auto copy = make(r);
		genmc::BitVector other;
		other.swap(copy);
		v.swap(other);
		break;
	}
	}
}

/* The answers of the find functions on the reference (-1: none) */
auto refFind(const Ref &r, unsigned from, unsigned to, bool value, bool last) -> int
{
	for (auto k = 0U; k < to - from; k++) {
		auto i = last ? to - 1 - k : from + k;
		if (r[i] == value)
			return static_cast<int>(i);
	}
	return -1;
}

void expectSame(const genmc::BitVector &v, const Ref &r)
{
	auto n = static_cast<unsigned>(r.size());
	RC_ASSERT(v.size() == n);
	RC_ASSERT(v.empty() == (n == 0));
	auto ones = static_cast<unsigned>(std::count(r.begin(), r.end(), true));
	RC_ASSERT(v.count() == ones);
	RC_ASSERT(v.any() == (ones > 0));
	RC_ASSERT(v.none() == (ones == 0));
	RC_ASSERT(v.all() == (ones == n));
	for (auto i = 0U; i < n; i++)
		RC_ASSERT(v[i] == r[i]);
	if (n)
		RC_ASSERT(v.back() == r.back());

	RC_ASSERT(v.find_first() == refFind(r, 0, n, true, false));
	RC_ASSERT(v.find_last() == refFind(r, 0, n, true, true));
	RC_ASSERT(v.find_first_unset() == refFind(r, 0, n, false, false));
	RC_ASSERT(v.find_last_unset() == refFind(r, 0, n, false, true));
	for (auto i = 0U; i < n; i++) {
		RC_ASSERT(v.find_next(i) == refFind(r, i + 1, n, true, false));
		RC_ASSERT(v.find_next_unset(i) == refFind(r, i + 1, n, false, false));
		RC_ASSERT(v.find_prev(i) == refFind(r, 0, i, true, true));
		RC_ASSERT(v.find_prev_unset(i) == refFind(r, 0, i, false, true));
	}

	std::vector<unsigned> setBits;
	for (auto i : v.set_bits())
		setBits.push_back(i);
	std::vector<unsigned> refBits;
	for (auto i = 0U; i < n; i++)
		if (r[i])
			refBits.push_back(i);
	RC_ASSERT(setBits == refBits);

	RC_ASSERT(v == make(r));
}

auto genOp() -> rc::Gen<Op>
{
	return rc::gen::build<Op>(
		rc::gen::set(&Op::kind, rc::gen::cast<Kind>(rc::gen::inRange<int>(
						0, static_cast<int>(Kind::Last) + 1))),
		rc::gen::set(&Op::a, rc::gen::inRange(0U, 1000U)),
		rc::gen::set(&Op::b, rc::gen::inRange(0U, 1000U)),
		rc::gen::set(&Op::bit, rc::gen::arbitrary<bool>()),
		rc::gen::set(&Op::other,
			     rc::gen::container<std::vector<bool>>(rc::gen::arbitrary<bool>())));
}

} // namespace

RC_GTEST_PROP(BitVector, AgreesWithAVectorOfBools, ())
{
	auto init = *rc::gen::inRange(0U, 200U);
	auto fill = *rc::gen::arbitrary<bool>();
	genmc::BitVector v(init, fill);
	Ref r(init, fill);
	expectSame(v, r);
	for (const auto &op : *rc::gen::container<std::vector<Op>>(genOp())) {
		apply(op, v, r);
		expectSame(v, r);
	}
}

TEST(BitVector, RangesAcrossWords)
{
	genmc::BitVector v(130);
	v.set(60, 70);
	EXPECT_EQ(v.count(), 10U);
	EXPECT_EQ(v.find_first_in(0, 130), 60);
	EXPECT_EQ(v.find_last_in(0, 130), 69);
	EXPECT_EQ(v.find_first_in(65, 130), 65);
	EXPECT_EQ(v.find_first_in(70, 130), -1);
	EXPECT_EQ(v.find_first_unset_in(60, 70), -1);
	EXPECT_EQ(v.find_last_unset_in(0, 70), 59);
	EXPECT_TRUE(v.anyCommon(genmc::BitVector(130, true)));
	genmc::BitVector sub(130);
	sub.set(61);
	EXPECT_FALSE(sub.test(v)); /* (test: bits of sub outside v) */
	sub.set(100);
	EXPECT_TRUE(sub.test(v));
}
