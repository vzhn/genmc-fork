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

#include "genmc/Verification/GraphHash.hpp"

#include "genmc/Execution/EventAttr.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"
#include "genmc/Support/SAddr.hpp"
#include "genmc/Support/SVal.hpp"

#include <algorithm>
#include <cstring>
#include <format>
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

	[[nodiscard]] auto name(int t) const -> std::string
	{
		return isLive(t) ? std::format("T{}", rank[t]) : std::format("T?{}", t);
	}

	[[nodiscard]] auto event(Event e) const -> std::string
	{
		if (e.isInitializer())
			return "init";
		return std::format("{}@{}", name(e.thread), e.index);
	}

	/* A dynamic address embeds the allocating thread's id (SAddr): rewrite it
	 * as the thread's rank. (A value that looks like one is rewritten the same
	 * way; small integers have thread 0, whose rank is 0.) */
	[[nodiscard]] auto addr(SAddr::Width a) const -> std::string
	{
		if (SAddr(a).isStatic())
			return std::format("G{:x}", a);
		auto t = static_cast<int>((a & SAddr::threadMask) >> SAddr::threadStartBit);
		if (!isLive(t))
			return std::format("D{:x}", a);
		auto storage = a & SAddr::storageMask;
		auto idx = a & SAddr::indexMask;
		return std::format("D{:x}:{}:{:x}", storage >> 60U, rank[t], idx);
	}
	[[nodiscard]] auto addr(SAddr a) const -> std::string { return addr(a.get()); }
	[[nodiscard]] auto value(SVal v) const -> std::string { return addr(v.get()); }
};

auto describeLabel(const ExecutionGraph &g, const EventLabel &lab, const ThreadNames &names,
		   const std::unordered_map<const EventLabel *, int> &coIndex) -> std::string
{
	auto s = std::format("{}:k{}:o{}", lab.getIndex(), static_cast<int>(lab.getKind()),
			     static_cast<int>(lab.getOrdering()));

	/* Accesses, allocations and frees: location and size */
	if (const auto *mLab = genmc::dyn_cast<MemLabel>(&lab))
		s += std::format(":a{}:s{}", names.addr(mLab->getAddr()), mLab->getSize().get());
	if (const auto *rLab = genmc::dyn_cast<ReadLabel>(&lab)) {
		if (rLab->getRf()) {
			s += std::format(
				":rf{}:v{}", names.event(rLab->getRf()->getPos()),
				names.value(rLab->getRf()->getAccessValue(rLab->getAccess())));
		} else {
			s += ":rf-";
		}
		if (const auto *cLab = genmc::dyn_cast<CasReadLabel>(&lab)) {
			s += std::format(":e{}:w{}:fo{}", names.value(cLab->getExpected()),
					 names.value(cLab->getSwapVal()),
					 static_cast<int>(cLab->getFailOrdering()));
			/* A weak CAS that fails spuriously performs no write; a
			 * failed one is the same read whichever flag it carries */
			if (cLab->isWeak() && cLab->failsSpuriously() && cLab->getRf() &&
			    cLab->valueMakesRMWSucceed(
				    cLab->getRf()->getAccessValue(cLab->getAccess())))
				s += ":spurious";
		}
		if (const auto *fLab = genmc::dyn_cast<FaiReadLabel>(&lab))
			s += std::format(":op{}:ov{}", static_cast<int>(fLab->getOp()),
					 names.value(fLab->getOpVal()));
	}
	if (const auto *wLab = genmc::dyn_cast<WriteLabel>(&lab)) {
		auto coIt = coIndex.find(&lab);
		/* WWRacy is set when the write is added, from the graph of that
		 * moment: not a property of this graph */
		auto attr = wLab->getAttr() & ~WriteAttr::WWRacy;
		s += std::format(":v{}:co{}:at{}", names.value(wLab->getVal()),
				 coIt == coIndex.end() ? -1 : coIt->second,
				 static_cast<unsigned>(attr));
	}
	if (const auto *fLab = genmc::dyn_cast<LkmmFenceLabel>(&lab))
		s += std::format(":f{}", static_cast<int>(fLab->getFenceKind()));
	if (const auto *cLab = genmc::dyn_cast<ThreadCreateLabel>(&lab))
		s += std::format(":c{}", names.name(cLab->getChildId()));
	if (const auto *jLab = genmc::dyn_cast<ThreadJoinLabel>(&lab))
		s += std::format(":j{}", names.name(jLab->getChildId()));
	if (const auto *bLab = genmc::dyn_cast<ThreadStartLabel>(&lab); bLab && lab.getIndex() == 0)
		s += std::format(":p{}", names.event(bLab->getCreateId()));
	if (const auto *eLab = genmc::dyn_cast<ThreadFinishLabel>(&lab))
		s += std::format(":r{}", names.value(eLab->getRetVal()));
	if (const auto *oLab = genmc::dyn_cast<OptionalLabel>(&lab))
		s += oLab->isExpanded() ? ":expanded" : ":folded";
	if (const auto *bLab = genmc::dyn_cast<ReadOptBlockLabel>(&lab))
		s += std::format(":a{}", names.addr(bLab->getAddr()));
	if (const auto *hLab = genmc::dyn_cast<HpProtectLabel>(&lab))
		s += std::format(":h{}:a{}", names.addr(hLab->getHpAddr()),
				 names.addr(hLab->getProtectedAddr()));
	if (const auto *hLab = genmc::dyn_cast<HelpingCasLabel>(&lab))
		s += std::format(":a{}:e{}:w{}", names.addr(hLab->getAddr()),
				 names.value(hLab->getExpected()), names.value(hLab->getSwapVal()));
	if (const auto *mLab = genmc::dyn_cast<MethodBeginLabel>(&lab))
		s += std::format(":m{}", mLab->getName());
	if (const auto *mLab = genmc::dyn_cast<MethodEndLabel>(&lab))
		s += std::format(":m{}", mLab->getName());
	if (const auto *oLab = genmc::dyn_cast<OutputLabel>(&lab))
		s += std::format(":msg{}", oLab->getMsg());
	if (const auto *eLab = genmc::dyn_cast<ErrorLabel>(&lab))
		s += std::format(":msg{}", eLab->getMsg());
	return s;
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
	for (auto t : order) {
		s += std::format("{}[", names.name(t));
		for (const auto &lab : g.po(t)) {
			s += describeLabel(g, lab, names, coIndex);
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
