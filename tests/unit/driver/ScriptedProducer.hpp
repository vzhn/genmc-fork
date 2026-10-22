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


#ifndef GENMC_TESTS_SCRIPTED_PRODUCER_HPP
#define GENMC_TESTS_SCRIPTED_PRODUCER_HPP

/*
 * An event producer for tests: a program written as a list of memory
 * operations per thread, run against GenMCDriver without the interpreter.
 * Main creates the threads in order and finishes; each thread runs its
 * operations and finishes. Variables are 4-byte globals, zero-initialized.
 */

#include "genmc/Verification/EventProducer.hpp"
#include "genmc/Verification/Explorer.hpp"
#include "genmc/Verification/GenMCDriver.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace scripted {

enum class OpKind : std::uint8_t { Load, Store, Fence, FetchAdd, CompareSwap };

struct Op {
	OpKind kind;
	unsigned var = 0;
	std::uint32_t val = 0;
	MemOrdering ord = MemOrdering::Relaxed;
	std::uint32_t expected = 0; /* CompareSwap: swaps in VAL if it reads this */
	bool blind = false;	    /* FetchAdd: its result is not used */

	auto operator==(const Op &) const -> bool = default;
};

inline auto ld(unsigned var, MemOrdering ord = MemOrdering::Relaxed) -> Op
{
	return {.kind = OpKind::Load, .var = var, .ord = ord};
}
inline auto st(unsigned var, std::uint32_t val, MemOrdering ord = MemOrdering::Relaxed) -> Op
{
	return {.kind = OpKind::Store, .var = var, .val = val, .ord = ord};
}
/** A fetch-and-add whose result the program does not use (a blind RMW:
 * its read is not part of the outcome, and -blind-rmw-reduction applies) */
inline auto blindFai(unsigned var, std::uint32_t val) -> Op
{
	return {.kind = OpKind::FetchAdd, .var = var, .val = val, .blind = true};
}

/** An atomic fetch-and-add of VAL (its read's value is part of the outcome) */
inline auto fai(unsigned var, std::uint32_t val, MemOrdering ord = MemOrdering::Relaxed) -> Op
{
	return {.kind = OpKind::FetchAdd, .var = var, .val = val, .ord = ord};
}

/** A strong compare-and-swap of VAL for EXPECTED (its read's value is part
 * of the outcome) */
inline auto cas(unsigned var, std::uint32_t expected, std::uint32_t val,
		MemOrdering ord = MemOrdering::Relaxed) -> Op
{
	return {.kind = OpKind::CompareSwap, .var = var, .val = val, .ord = ord, .expected = expected};
}

inline auto fence(MemOrdering ord = MemOrdering::SequentiallyConsistent) -> Op
{
	return {.kind = OpKind::Fence, .ord = ord};
}

struct Program {
	unsigned numVars = 0;
	std::vector<std::vector<Op>> threads;
};

/** One complete execution: the values each thread read, in program order
 * (but those of blind RMWs), and last the final value of each variable */
using Outcome = std::vector<std::vector<std::uint32_t>>;

/** The reductions an exploration uses */
struct Options {
	bool symmetryReduction = false;
	bool blindRMWReduction = false;
};

class Producer : public EventProducer {
public:
	static constexpr std::uint32_t kSize = 4;

	Producer(GenMCDriver &driver, Program prog)
		: driver_(driver), prog_(std::move(prog)), init_(prog_.numVars, 0U)
	{
		for (auto i = 0U; i < prog_.numVars; i++)
			addrs_.push_back(driver_.allocateGlobal(ASize(kSize), kSize, &init_[i]));
	}

	void runFromStart(const std::vector<ThreadInfo> &tis, bool /*collectDbg*/) override
	{
		acts_.clear();
		funOf_.assign(1, 0);
		pc_.assign(1, 0);
		acts_.emplace_back(ActionKind::NonLoad, Event(0, 0));
		for (const auto &ti : tis)
			addThread(ti.id, static_cast<unsigned>(ti.arg.get()));

		auto next = driver_.scheduleNext(acts_);
		while (const auto *tid = std::get_if<int>(&next)) {
			step(*tid);
			next = driver_.scheduleNext(acts_);
		}
		if (std::holds_alternative<GenMCDriver::Finished>(next))
			outcomes.insert(outcomeOf(driver_.getCurrentGraph()));
		++runs;
	}

	/** The outcomes of the complete executions explored */
	std::set<Outcome> outcomes;

	/** How many times the program ran */
	unsigned runs = 0;

private:
	/* The values the reads of complete graph G saw. (Read from the graph:
	 * the driver does not replay a thread that already finished, so a run
	 * does not see every read.) */
	auto outcomeOf(const ExecutionGraph &g) const -> Outcome
	{
		Outcome out(prog_.threads.size() + 1);
		for (auto t = 1U; t < g.getNumThreads(); t++) {
			auto prog = g.getFirstThreadLabel(t)->getThreadInfo().arg.get();
			for (auto i = 0U; i < g.getThreadSize(t); i++) {
				const auto *lab = g.getEventLabel(Event(static_cast<int>(t), static_cast<int>(i)));
				const auto *rLab = genmc::dyn_cast<ReadLabel>(lab);
				if (!rLab)
					continue;
				const auto *fLab = genmc::dyn_cast<FaiReadLabel>(rLab);
				if (fLab && fLab->hasAttr(WriteAttr::Blind))
					continue;
				out[prog - 1].push_back(static_cast<std::uint32_t>(rLab->getReturnValue().get()));
			}
		}
		for (auto v = 0U; v < prog_.numVars; v++) {
			const auto *wLab = g.containsLoc(addrs_[v])
						   ? genmc::dyn_cast<WriteLabel>(g.co_max(addrs_[v]))
						   : nullptr; /* never written: the initial 0 */
			out.back().push_back(wLab ? static_cast<std::uint32_t>(wLab->getVal().get()) : 0U);
		}
		return out;
	}

	void addThread(int tid, unsigned funId)
	{
		while (acts_.size() <= static_cast<size_t>(tid)) {
			acts_.emplace_back(ActionKind::NonLoad, Event(static_cast<int>(acts_.size()), 0));
			funOf_.push_back(0);
			pc_.push_back(0);
		}
		funOf_[tid] = funId;
		updateKind(tid);
	}

	auto ops(int tid) const -> const std::vector<Op> & { return prog_.threads[funOf_[tid] - 1]; }

	void updateKind(int tid)
	{
		if (tid == 0)
			return;
		const auto &o = ops(tid);
		acts_[tid].kind = (pc_[tid] < o.size() && (o[pc_[tid]].kind == OpKind::Load ||
							     o[pc_[tid]].kind == OpKind::FetchAdd ||
							     o[pc_[tid]].kind == OpKind::CompareSwap))
					  ? ActionKind::Load
					  : ActionKind::NonLoad;
	}

	template <typename T> auto advance(int tid, const GenMCDriver::HandleResult<T> &res) -> const T *
	{
		acts_[tid].event.index += static_cast<int>(res.count);
		const auto *val = std::get_if<T>(&res.result);
		if (val) {
			++pc_[tid];
			updateKind(tid);
		}
		return val;
	}

	void step(int tid)
	{
		const auto pos = acts_[tid].event;
		if (tid == 0) {
			if (pc_[0] < prog_.threads.size()) {
				/* Identical threads run the same function (so that symmetry
				 * reduction sees them); the argument, unused, names the thread */
				auto prog = static_cast<unsigned>(pc_[0] + 1);
				auto fun = 1U + static_cast<unsigned>(std::distance(
						   prog_.threads.begin(),
						   std::ranges::find(prog_.threads, prog_.threads[prog - 1])));
				ThreadInfo info(-1, 0, fun, SVal(prog), "t" + std::to_string(prog));
				info.argUsed = false;
				if (const auto *child = advance(0, driver_.handleThreadCreate(
									   nullptr, pos, info, EventDeps())))
					addThread(*child, prog);
			} else {
				advance(0, driver_.handleThreadFinish(nullptr, pos, SVal(0)));
			}
			return;
		}

		const auto &o = ops(tid);
		if (pc_[tid] == o.size()) {
			advance(tid, driver_.handleThreadFinish(nullptr, pos, SVal(0)));
			return;
		}
		const auto &op = o[pc_[tid]];
		switch (op.kind) {
		case OpKind::Load:
			advance(tid, driver_.handleRead(nullptr, pos, op.ord, addrs_[op.var],
							ASize(kSize), nullptr, std::nullopt, EventDeps()));
			break;
		case OpKind::Store:
			advance(tid, driver_.handleWrite(nullptr, pos, op.ord, addrs_[op.var],
							 ASize(kSize), SVal(op.val), WriteAttr::None,
							 EventDeps()));
			break;
		case OpKind::Fence:
			advance(tid, driver_.handleFence(nullptr, pos, op.ord, EventDeps()));
			break;
		case OpKind::CompareSwap: {
			/* As the interpreter issues a strong cmpxchg: the CAS read, and
			 * the CAS write only if the read succeeds */
			auto r = driver_.handleCasRead(nullptr, pos, op.ord, op.ord, addrs_[op.var],
						       ASize(kSize), SVal(op.expected), SVal(op.val),
						       WriteAttr::None, false, nullptr, std::nullopt,
						       EventDeps());
			acts_[tid].event.index += static_cast<int>(r.count);
			const auto *res = std::get_if<GenMCDriver::CasReadResult>(&r.result);
			if (!res)
				break;
			if (!res->success) {
				++pc_[tid];
				updateKind(tid);
				break;
			}
			advance(tid, driver_.handleCasWrite(nullptr, acts_[tid].event, op.ord,
							    addrs_[op.var], ASize(kSize), SVal(op.val),
							    WriteAttr::None, EventDeps()));
			break;
		}
		case OpKind::FetchAdd: {
			/* One instruction, two events (as the interpreter issues an
			 * atomicrmw): the read, then the write of the sum. If either is
			 * reset, the instruction runs again */
			auto wattr = op.blind ? WriteAttr::Blind : WriteAttr::None;
			auto r = driver_.handleFaiRead(nullptr, pos, op.ord, addrs_[op.var],
						       ASize(kSize), RMWBinOp::Add, SVal(op.val), wattr,
						       nullptr, std::nullopt, EventDeps());
			acts_[tid].event.index += static_cast<int>(r.count);
			const auto *old = std::get_if<SVal>(&r.result);
			if (!old)
				break;
			auto sum = static_cast<std::uint32_t>(old->get() + op.val);
			advance(tid, driver_.handleFaiWrite(nullptr, acts_[tid].event, op.ord,
							    addrs_[op.var], ASize(kSize), SVal(sum), wattr,
							    EventDeps()));
			break;
		}
		}
	}

	GenMCDriver &driver_;
	Program prog_;
	std::vector<std::uint32_t> init_;
	std::vector<SAddr> addrs_;

	/* Per run: each thread's position, function and next operation */
	std::vector<Action> acts_;
	std::vector<unsigned> funOf_;
	std::vector<size_t> pc_;
};

/** The result of exploring PROG under MODEL */
struct Exploration {
	unsigned explored = 0;
	unsigned blocked = 0;
	bool error = false;
	std::set<Outcome> outcomes;
};

inline auto explore(const Program &prog, ModelType model, Options opts = {}) -> Exploration
{
	auto conf = std::make_shared<Config>();
	conf->model = model;
	conf->isDepTrackingModel = (model == ModelType::IMM || model == ModelType::LKMM);
	conf->schedulePolicy = SchedulePolicy::WF;
	conf->randomScheduleSeed = 0;
	conf->warnOnGraphSize = 1024;
	conf->symmetryReduction = opts.symmetryReduction;
	conf->blindRMWReduction = opts.blindRMWReduction;
	auto driver = GenMCDriver::create(conf);
	Producer producer(*driver, prog);
	::explore(*driver, producer);
	const auto &res = driver->getResult();
	return {.explored = res.explored,
		.blocked = res.exploredBlocked,
		.error = res.status.has_value(),
		.outcomes = producer.outcomes};
}

} // namespace scripted

#endif /* GENMC_TESTS_SCRIPTED_PRODUCER_HPP */
