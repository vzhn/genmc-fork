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

#include "genmc/Verification/Checks/GraphHash.hpp"

#include "genmc/Execution/EventAttr.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Support/SAddr.hpp"
#include "genmc/Support/SVal.hpp"

#include <algorithm>
#include <charconv>
#include <cstring>
#include <unordered_map>
#include <vector>

/************************************************************
 ** MurmurHash3_x64_128 (Austin Appleby, public domain)
 ***********************************************************/

namespace {

inline auto rotl64(std::uint64_t x, int r) -> std::uint64_t { return (x << r) | (x >> (64 - r)); }

inline auto fmix64(std::uint64_t k) -> std::uint64_t
{
	k ^= k >> 33;
	k *= 0xff51afd7ed558ccdULL;
	k ^= k >> 33;
	k *= 0xc4ceb9fe1a85ec53ULL;
	k ^= k >> 33;
	return k;
}

inline auto getblock64(const std::uint8_t *p, std::size_t i) -> std::uint64_t
{
	std::uint64_t k = 0;
	std::memcpy(&k, p + (i * 8), sizeof(k));
	return k;
}

} // namespace

auto murmurHash128(const void *data, std::size_t len, std::uint64_t seed) -> GraphHash
{
	const auto *bytes = static_cast<const std::uint8_t *>(data);
	const auto nblocks = len / 16;

	std::uint64_t h1 = seed;
	std::uint64_t h2 = seed;
	const std::uint64_t c1 = 0x87c37b91114253d5ULL;
	const std::uint64_t c2 = 0x4cf5ad432745937fULL;

	for (std::size_t i = 0; i < nblocks; i++) {
		auto k1 = getblock64(bytes, i * 2 + 0);
		auto k2 = getblock64(bytes, i * 2 + 1);

		k1 *= c1;
		k1 = rotl64(k1, 31);
		k1 *= c2;
		h1 ^= k1;
		h1 = rotl64(h1, 27);
		h1 += h2;
		h1 = h1 * 5 + 0x52dce729;

		k2 *= c2;
		k2 = rotl64(k2, 33);
		k2 *= c1;
		h2 ^= k2;
		h2 = rotl64(h2, 31);
		h2 += h1;
		h2 = h2 * 5 + 0x38495ab5;
	}

	const auto *tail = bytes + (nblocks * 16);
	std::uint64_t k1 = 0;
	std::uint64_t k2 = 0;
	switch (len & 15) {
	case 15:
		k2 ^= static_cast<std::uint64_t>(tail[14]) << 48;
		[[fallthrough]];
	case 14:
		k2 ^= static_cast<std::uint64_t>(tail[13]) << 40;
		[[fallthrough]];
	case 13:
		k2 ^= static_cast<std::uint64_t>(tail[12]) << 32;
		[[fallthrough]];
	case 12:
		k2 ^= static_cast<std::uint64_t>(tail[11]) << 24;
		[[fallthrough]];
	case 11:
		k2 ^= static_cast<std::uint64_t>(tail[10]) << 16;
		[[fallthrough]];
	case 10:
		k2 ^= static_cast<std::uint64_t>(tail[9]) << 8;
		[[fallthrough]];
	case 9:
		k2 ^= static_cast<std::uint64_t>(tail[8]) << 0;
		k2 *= c2;
		k2 = rotl64(k2, 33);
		k2 *= c1;
		h2 ^= k2;
		[[fallthrough]];
	case 8:
		k1 ^= static_cast<std::uint64_t>(tail[7]) << 56;
		[[fallthrough]];
	case 7:
		k1 ^= static_cast<std::uint64_t>(tail[6]) << 48;
		[[fallthrough]];
	case 6:
		k1 ^= static_cast<std::uint64_t>(tail[5]) << 40;
		[[fallthrough]];
	case 5:
		k1 ^= static_cast<std::uint64_t>(tail[4]) << 32;
		[[fallthrough]];
	case 4:
		k1 ^= static_cast<std::uint64_t>(tail[3]) << 24;
		[[fallthrough]];
	case 3:
		k1 ^= static_cast<std::uint64_t>(tail[2]) << 16;
		[[fallthrough]];
	case 2:
		k1 ^= static_cast<std::uint64_t>(tail[1]) << 8;
		[[fallthrough]];
	case 1:
		k1 ^= static_cast<std::uint64_t>(tail[0]) << 0;
		k1 *= c1;
		k1 = rotl64(k1, 31);
		k1 *= c2;
		h1 ^= k1;
		break;
	default:
		break;
	}

	h1 ^= len;
	h2 ^= len;
	h1 += h2;
	h2 += h1;
	h1 = fmix64(h1);
	h2 = fmix64(h2);
	h1 += h2;
	h2 += h1;
	return {.lo = h1, .hi = h2};
}

/************************************************************
 ** Canonical description
 ***********************************************************/

namespace {

/* (The description is built with std::to_chars, not std::format: it is
 * computed for every graph a revisit produces and every complete execution,
 * and formatting was most of its cost, issue #261) */
template <typename T> void appendInt(std::string &s, T v)
{
	char buf[24];
	auto [end, ec] = std::to_chars(buf, buf + sizeof(buf), v);
	s.append(buf, end);
}

template <typename T> void appendHex(std::string &s, T v)
{
	char buf[24];
	auto [end, ec] = std::to_chars(buf, buf + sizeof(buf), v, 16);
	s.append(buf, end);
}

/* Thread naming and the rewriting of thread ids embedded in addresses */
struct ThreadNames {
	/* The creation path of each thread (empty for the main thread) */
	std::vector<std::vector<int>> paths;
	/* The rank of each thread in the order of the paths (-1: left out) */
	std::vector<int> rank;

	explicit ThreadNames(const ExecutionGraph &g)
		: paths(g.getNumThreads()), rank(g.getNumThreads(), -1)
	{
		std::vector<int> live;
		for (auto t : g.thr_ids()) {
			if (g.isThreadEmpty(t))
				continue;
			if (t == 0) {
				live.push_back(t);
				continue;
			}
			const auto *bLab =
				genmc::dyn_cast<ThreadStartLabel>(g.getFirstThreadLabel(t));
			if (!bLab)
				continue;
			/* A begin whose create was cut is a terminated placeholder
			 * (revived by the same create), not a thread of the graph */
			auto cid = bLab->getCreateId();
			const auto *cLab = genmc::dyn_cast_if_present<ThreadCreateLabel>(
				g.getEventLabelIfPresent(cid));
			if (!cLab || cLab->getChildId() != t)
				continue;
			/* The parent was created first, so its path is known */
			paths[t] = paths[cid.thread];
			paths[t].push_back(cid.index);
			live.push_back(t);
		}
		std::ranges::sort(live, [&](int a, int b) { return paths[a] < paths[b]; });
		for (auto i = 0U; i < live.size(); i++)
			rank[live[i]] = static_cast<int>(i);
	}

	[[nodiscard]] auto isLive(int t) const -> bool
	{
		return t >= 0 && t < static_cast<int>(rank.size()) && rank[t] >= 0;
	}

	void name(std::string &s, int t) const
	{
		if (isLive(t)) {
			s += 'T';
			appendInt(s, rank[t]);
		} else {
			s += "T?";
			appendInt(s, t);
		}
	}

	void event(std::string &s, Event e) const
	{
		if (e.isInitializer()) {
			s += "init";
			return;
		}
		name(s, e.thread);
		s += '@';
		appendInt(s, e.index);
	}

	/* A dynamic address embeds the allocating thread's id (SAddr), and so
	 * does another thread's copy of a thread-local: rewrite it as the
	 * thread's rank. (A value that looks like one is rewritten the same way;
	 * small integers have thread 0, whose rank is 0.) */
	void addr(std::string &s, SAddr::Width a) const
	{
		auto t = SAddr(a).getThread();
		if (SAddr(a).isStatic()) {
			s += 'G';
			if (t == 0 || !isLive(t)) {
				appendHex(s, a);
				return;
			}
			appendHex(s, SAddr(a).withThread(0).get());
			s += ':';
			appendInt(s, rank[t]);
			return;
		}
		s += 'D';
		if (!isLive(t)) {
			appendHex(s, a);
			return;
		}
		auto storage = a & SAddr::storageMask;
		auto idx = a & SAddr::indexMask;
		appendHex(s, storage >> 60U);
		s += ':';
		appendInt(s, rank[t]);
		s += ':';
		appendHex(s, idx);
	}
	void addr(std::string &s, SAddr a) const { addr(s, a.get()); }
	void value(std::string &s, SVal v) const { addr(s, v.get()); }
};

void describeLabel(std::string &s, const ExecutionGraph &g, const EventLabel &lab,
		   const ThreadNames &names,
		   const std::unordered_map<const EventLabel *, int> &coIndex)
{
	appendInt(s, lab.getIndex());
	s += ":k";
	appendInt(s, static_cast<int>(lab.getKind()));
	s += ":o";
	appendInt(s, static_cast<int>(lab.getOrdering()));

	/* Accesses, allocations and frees: location and size */
	if (const auto *mLab = genmc::dyn_cast<MemLabel>(&lab)) {
		s += ":a";
		names.addr(s, mLab->getAddr());
		s += ":s";
		appendInt(s, mLab->getSize().get());
	}
	if (const auto *rLab = genmc::dyn_cast<ReadLabel>(&lab)) {
		/* (A read of the initializer of dynamic memory not initialized
		 * yet has no value: a run that found it an error was dropped, as
		 * a thread had events kept after a hole, see reportError()) */
		const auto *rf = rLab->getRf();
		const auto uninit = rf && rf->getPos().isInitializer() &&
				    rLab->getAddr().isDynamic() &&
				    !g.getState().isInitialized(rLab->getAccess());
		if (rf && uninit) {
			s += ":rf";
			names.event(s, rf->getPos());
			s += ":v?";
		} else if (rf) {
			s += ":rf";
			names.event(s, rf->getPos());
			s += ":v";
			names.value(s, rf->getAccessValue(rLab->getAccess()));
		} else {
			s += ":rf-";
		}
		if (const auto *cLab = genmc::dyn_cast<CasReadLabel>(&lab)) {
			s += ":e";
			names.value(s, cLab->getExpected());
			s += ":w";
			names.value(s, cLab->getSwapVal());
			s += ":fo";
			appendInt(s, static_cast<int>(cLab->getFailOrdering()));
			/* A weak CAS that fails spuriously performs no write; a
			 * failed one is the same read whichever flag it carries */
			if (cLab->isWeak() && cLab->failsSpuriously() && cLab->getRf() &&
			    cLab->valueMakesRMWSucceed(
				    cLab->getRf()->getAccessValue(cLab->getAccess())))
				s += ":spurious";
		}
		if (const auto *fLab = genmc::dyn_cast<FaiReadLabel>(&lab)) {
			s += ":op";
			appendInt(s, static_cast<int>(fLab->getOp()));
			s += ":ov";
			names.value(s, fLab->getOpVal());
		}
	}
	if (const auto *wLab = genmc::dyn_cast<WriteLabel>(&lab)) {
		auto coIt = coIndex.find(&lab);
		/* WWRacy is set when the write is added, from the graph of that
		 * moment: not a property of this graph */
		auto attr = wLab->getAttr() & ~WriteAttr::WWRacy;
		s += ":v";
		names.value(s, wLab->getVal());
		s += ":co";
		appendInt(s, coIt == coIndex.end() ? -1 : coIt->second);
		s += ":at";
		appendInt(s, static_cast<unsigned>(attr));
	}
	if (const auto *fLab = genmc::dyn_cast<LkmmFenceLabel>(&lab)) {
		s += ":f";
		appendInt(s, static_cast<int>(fLab->getFenceKind()));
	}
	if (const auto *cLab = genmc::dyn_cast<ThreadCreateLabel>(&lab)) {
		s += ":c";
		names.name(s, cLab->getChildId());
	}
	if (const auto *jLab = genmc::dyn_cast<ThreadJoinLabel>(&lab)) {
		s += ":j";
		names.name(s, jLab->getChildId());
	}
	if (const auto *bLab = genmc::dyn_cast<ThreadStartLabel>(&lab);
	    bLab && lab.getIndex() == 0) {
		s += ":p";
		names.event(s, bLab->getCreateId());
	}
	if (const auto *eLab = genmc::dyn_cast<ThreadFinishLabel>(&lab)) {
		s += ":r";
		names.value(s, eLab->getRetVal());
	}
	if (const auto *oLab = genmc::dyn_cast<OptionalLabel>(&lab))
		s += oLab->isExpanded() ? ":expanded" : ":folded";
	if (const auto *bLab = genmc::dyn_cast<ReadOptBlockLabel>(&lab)) {
		s += ":a";
		names.addr(s, bLab->getAddr());
	}
	if (const auto *hLab = genmc::dyn_cast<HpProtectLabel>(&lab)) {
		s += ":h";
		names.addr(s, hLab->getHpAddr());
		s += ":a";
		names.addr(s, hLab->getProtectedAddr());
	}
	if (const auto *hLab = genmc::dyn_cast<HelpingCasLabel>(&lab)) {
		s += ":a";
		names.addr(s, hLab->getAddr());
		s += ":e";
		names.value(s, hLab->getExpected());
		s += ":w";
		names.value(s, hLab->getSwapVal());
	}
	if (const auto *mLab = genmc::dyn_cast<MethodBeginLabel>(&lab)) {
		s += ":m";
		s += mLab->getName();
	}
	if (const auto *mLab = genmc::dyn_cast<MethodEndLabel>(&lab)) {
		s += ":m";
		s += mLab->getName();
	}
	if (const auto *oLab = genmc::dyn_cast<OutputLabel>(&lab)) {
		s += ":msg";
		s += oLab->getMsg();
	}
	if (const auto *eLab = genmc::dyn_cast<ErrorLabel>(&lab)) {
		s += ":msg";
		s += eLab->getMsg();
	}
}

} // namespace

auto canonicalGraphDescription(const ExecutionGraph &g) -> std::string
{
	ThreadNames names(g);

	/* The position of each write in its location's coherence order */
	std::unordered_map<const EventLabel *, int> coIndex;
	for (auto lIt = g.loc_begin(), lE = g.loc_end(); lIt != lE; ++lIt) {
		auto i = 0;
		for (const auto &wLab : g.co(lIt->first))
			coIndex[&wLab] = i++;
	}

	std::vector<int> order;
	for (auto t : g.thr_ids())
		if (names.isLive(t))
			order.push_back(t);
	std::ranges::sort(order, [&](int a, int b) { return names.rank[a] < names.rank[b]; });

	std::string s;
	s.reserve(64 * (g.getMaxStamp().get() + 1));
	for (auto t : order) {
		names.name(s, t);
		s += '[';
		for (const auto &lab : g.po(t)) {
			describeLabel(s, g, lab, names, coIndex);
			s += ';';
		}
		s += "]\n";
	}
	return s;
}

auto canonicalGraphHash(const ExecutionGraph &g) -> GraphHash
{
	auto s = canonicalGraphDescription(g);
	return murmurHash128(s.data(), s.size());
}

