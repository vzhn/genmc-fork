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


#ifndef GENMC_TESTS_AXIOMATIC_ORACLE_HPP
#define GENMC_TESTS_AXIOMATIC_ORACLE_HPP

/*
 * A brute-force reference for scripted programs without fences: enumerate
 * every reads-from choice and every coherence order, and keep the graphs
 * the model's axioms allow. Shares no code with the driver.
 *
 *   SC:            acyclic(po | rf | co | fr)
 *   RC11, relaxed: acyclic(po-loc | rf | co | fr) per location
 *                  and acyclic(po | rf)            (no out-of-thin-air)
 *   both:          atomicity: an RMW's write comes right after, in co,
 *                  the write its read reads from
 *
 * An RMW is a read and a write of the same location. A fetch-and-add's
 * write is the value its read reads plus the operand; a (strong) CAS has
 * its write only if its read reads the expected value.
 *
 * The RC11 rule holds only when every access is relaxed (no hb beyond po).
 */

#include "ScriptedProducer.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <numeric>

namespace oracle {

struct Result {
	unsigned graphs = 0;
	std::set<scripted::Outcome> outcomes;
};

namespace detail {

struct Ev {
	int thr;  /* -1: initializing write */
	int fun;  /* the thread's index in the program */
	int idx;  /* position in the thread */
	bool write;
	unsigned var;
	std::uint32_t val;
	int rmwRead = -1; /* the write of an RMW: the event of its read */
	bool cas = false;  /* the write of a CAS: only if its read reads EXPECTED */
	std::uint32_t expected = 0;
};

/* Whether the graph on N nodes with edges E is acyclic */
inline auto acyclic(unsigned n, const std::vector<std::pair<int, int>> &e) -> bool
{
	std::vector<std::vector<int>> succ(n);
	std::vector<int> indeg(n, 0);
	for (auto [a, b] : e) {
		succ[a].push_back(b);
		++indeg[b];
	}
	std::vector<int> ready;
	for (auto i = 0U; i < n; i++)
		if (indeg[i] == 0)
			ready.push_back(static_cast<int>(i));
	unsigned seen = 0;
	while (!ready.empty()) {
		auto v = ready.back();
		ready.pop_back();
		++seen;
		for (auto s : succ[v])
			if (--indeg[s] == 0)
				ready.push_back(s);
	}
	return seen == n;
}

} // namespace detail

inline auto enumerate(const scripted::Program &prog, ModelType model) -> Result
{
	using detail::Ev;
	std::vector<Ev> evs;
	for (auto v = 0U; v < prog.numVars; v++)
		evs.push_back({.thr = -1, .fun = -1, .idx = 0, .write = true, .var = v, .val = 0});
	for (auto t = 0U; t < prog.threads.size(); t++) {
		auto idx = 0;
		for (const auto &op : prog.threads[t]) {
			VERIFY(op.kind != scripted::OpKind::Fence);
			auto isRMW = op.kind == scripted::OpKind::FetchAdd ||
				     op.kind == scripted::OpKind::CompareSwap;
			evs.push_back({.thr = static_cast<int>(t),
				       .fun = static_cast<int>(t),
				       .idx = idx++,
				       .write = op.kind == scripted::OpKind::Store,
				       .var = op.var,
				       .val = op.val});
			if (isRMW)
				evs.push_back({.thr = static_cast<int>(t),
					       .fun = static_cast<int>(t),
					       .idx = idx++,
					       .write = true,
					       .var = op.var,
					       .val = op.val, /* the operand, or what a CAS swaps in */
					       .rmwRead = static_cast<int>(evs.size()) - 1,
					       .cas = op.kind == scripted::OpKind::CompareSwap,
					       .expected = op.expected});
		}
	}
	const auto n = static_cast<unsigned>(evs.size());

	std::vector<int> reads;
	std::vector<std::vector<int>> writesOf(prog.numVars); /* non-init writes */
	for (auto i = 0U; i < n; i++) {
		if (!evs[i].write)
			reads.push_back(static_cast<int>(i));
		else if (evs[i].thr >= 0)
			writesOf[evs[i].var].push_back(static_cast<int>(i));
	}

	/* po: init writes before everything, then program order */
	std::vector<std::pair<int, int>> po;
	for (auto a = 0U; a < n; a++)
		for (auto b = 0U; b < n; b++)
			if (a != b && ((evs[a].thr == -1 && evs[b].thr >= 0) ||
				       (evs[a].thr >= 0 && evs[a].thr == evs[b].thr &&
					evs[a].idx < evs[b].idx)))
				po.emplace_back(a, b);

	Result res;
	std::vector<int> rf(reads.size());
	std::vector<int> rfOf(n, -1);
	std::vector<std::uint32_t> value(n, 0);
	std::vector<bool> exists(n, true);
	std::vector<std::vector<int>> co(prog.numVars); /* co order of non-init writes */

	/* Given rf: the value of each write and whether it exists (a CAS writes
	 * iff its read reads the expected value; an RMW's write is computed from
	 * what its read reads). False if rf has a cycle through RMWs (atomicity
	 * rules such graphs out) or reads a write that does not exist */
	auto evaluate = [&]() -> bool {
		std::fill(rfOf.begin(), rfOf.end(), -1);
		for (auto k = 0U; k < reads.size(); k++)
			rfOf[reads[k]] = rf[k];
		std::vector<int> state(n, 0); /* 0 new, 1 in progress, 2 done */
		std::function<bool(int)> eval = [&](int w) -> bool {
			if (state[w] == 2)
				return true;
			if (state[w] == 1)
				return false;
			state[w] = 1;
			exists[w] = true;
			value[w] = evs[w].val;
			if (evs[w].rmwRead >= 0) {
				auto src = rfOf[evs[w].rmwRead];
				if (!eval(src) || !exists[src])
					return false;
				if (evs[w].cas)
					exists[w] = value[src] == evs[w].expected;
				else
					value[w] = value[src] + evs[w].val;
			}
			state[w] = 2;
			return true;
		};
		for (auto i = 0U; i < n; i++)
			if (evs[i].write && !eval(static_cast<int>(i)))
				return false;
		return std::ranges::all_of(rf, [&](int w) { return exists[w]; });
	};

	auto check = [&]() {
		/* co edges (init first) and position in co */
		std::vector<int> coPos(n, 0);
		std::vector<std::pair<int, int>> coE;
		for (auto v = 0U; v < prog.numVars; v++) {
			int prev = static_cast<int>(v); /* the init write of v */
			for (auto k = 0U; k < co[v].size(); k++) {
				coE.emplace_back(prev, co[v][k]);
				coPos[co[v][k]] = static_cast<int>(k) + 1;
				prev = co[v][k];
			}
		}
		std::vector<std::pair<int, int>> rfE, frE;
		for (auto k = 0U; k < reads.size(); k++) {
			auto r = reads[k];
			auto w = rf[k];
			rfE.emplace_back(w, r);
			for (auto w2 : co[evs[r].var])
				if (coPos[w2] > coPos[w])
					frE.emplace_back(r, w2);
		}
		/* (A failed CAS has no write: no edges to or from it) */
		auto all = [&](auto keep) {
			std::vector<std::pair<int, int>> e;
			for (const auto *rel : {&po, &rfE, &coE, &frE})
				for (auto p : *rel)
					if (exists[p.first] && exists[p.second] && keep(rel, p))
						e.push_back(p);
			return e;
		};
		/* Atomicity: an RMW's write right after its read's write in co */
		for (auto w = 0U; w < n; w++) {
			if (evs[w].rmwRead < 0 || !exists[w])
				continue;
			if (coPos[w] != coPos[rfOf[evs[w].rmwRead]] + 1)
				return;
		}

		bool ok = true;
		if (model == ModelType::SC) {
			ok = detail::acyclic(n, all([](auto *, auto) { return true; }));
		} else {
			for (auto v = 0U; ok && v < prog.numVars; v++)
				ok = detail::acyclic(n, all([&](auto *rel, auto p) {
					return evs[p.first].var == v && evs[p.second].var == v;
				}));
			ok = ok && detail::acyclic(n, all([&](auto *rel, auto) {
				return rel == &po || rel == &rfE;
			}));
		}
		if (!ok)
			return;
		++res.graphs;
		scripted::Outcome out(prog.threads.size());
		for (auto k = 0U; k < reads.size(); k++)
			out[evs[reads[k]].fun].push_back(value[rf[k]]);
		res.outcomes.insert(out);
	};

	std::function<void(unsigned)> chooseCo = [&](unsigned v) {
		if (v == prog.numVars) {
			check();
			return;
		}
		std::vector<int> perm;
		std::ranges::copy_if(writesOf[v], std::back_inserter(perm),
				     [&](int w) { return exists[w]; });
		std::ranges::sort(perm);
		do {
			co[v] = perm;
			chooseCo(v + 1);
		} while (std::ranges::next_permutation(perm).found);
	};
	std::function<void(unsigned)> chooseRf = [&](unsigned k) {
		if (k == reads.size()) {
			if (evaluate())
				chooseCo(0);
			return;
		}
		auto var = evs[reads[k]].var;
		rf[k] = static_cast<int>(var); /* init */
		chooseRf(k + 1);
		for (auto w : writesOf[var]) {
			rf[k] = w;
			chooseRf(k + 1);
		}
	};
	chooseRf(0);
	return res;
}

} // namespace oracle

#endif /* GENMC_TESTS_AXIOMATIC_ORACLE_HPP */
