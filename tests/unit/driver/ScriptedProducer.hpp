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

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace scripted {

enum class OpKind : std::uint8_t { Load, Store, Fence };

struct Op {
	OpKind kind;
	unsigned var = 0;
	std::uint32_t val = 0;
	MemOrdering ord = MemOrdering::Relaxed;
};

inline auto ld(unsigned var, MemOrdering ord = MemOrdering::Relaxed) -> Op
{
	return {.kind = OpKind::Load, .var = var, .ord = ord};
}
inline auto st(unsigned var, std::uint32_t val, MemOrdering ord = MemOrdering::Relaxed) -> Op
{
	return {.kind = OpKind::Store, .var = var, .val = val, .ord = ord};
}
inline auto fence(MemOrdering ord = MemOrdering::SequentiallyConsistent) -> Op
{
	return {.kind = OpKind::Fence, .ord = ord};
}

struct Program {
	unsigned numVars = 0;
	std::vector<std::vector<Op>> threads;
};

/** The values one complete execution read, per thread in program order */
using Outcome = std::vector<std::vector<std::uint32_t>>;

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
			addThread(ti.id, ti.funId);

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
		Outcome out(prog_.threads.size());
		for (auto t = 1U; t < g.getNumThreads(); t++) {
			auto fun = g.getFirstThreadLabel(t)->getThreadInfo().funId;
			for (auto i = 0U; i < g.getThreadSize(t); i++) {
				const auto *lab = g.getEventLabel(Event(static_cast<int>(t), static_cast<int>(i)));
				if (genmc::isa<ReadLabel>(lab))
					out[fun - 1].push_back(
						static_cast<std::uint32_t>(lab->getReturnValue().get()));
			}
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
		acts_[tid].kind = (pc_[tid] < o.size() && o[pc_[tid]].kind == OpKind::Load)
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
				auto fun = static_cast<unsigned>(pc_[0] + 1);
				ThreadInfo info(-1, 0, fun, SVal(0), "t" + std::to_string(fun));
				if (const auto *child = advance(0, driver_.handleThreadCreate(
									   nullptr, pos, info, EventDeps())))
					addThread(*child, fun);
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

inline auto explore(const Program &prog, ModelType model) -> Exploration
{
	auto conf = std::make_shared<Config>();
	conf->model = model;
	conf->isDepTrackingModel = (model == ModelType::IMM || model == ModelType::LKMM);
	conf->schedulePolicy = SchedulePolicy::WF;
	conf->randomScheduleSeed = 0;
	conf->warnOnGraphSize = 1024;
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
