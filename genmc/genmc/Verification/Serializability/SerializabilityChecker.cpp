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

#include "genmc/Verification/Serializability/SerializabilityChecker.hpp"
#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Execution/ExecutionGraph.hpp"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <format>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

/** A region (its events contracted), or an event outside every region */
struct Node {
	Event first;
	Event last;
	std::string name; /**< The region's name (empty for an event outside regions) */
	int ordinal = 0;  /**< The region's position among its thread's regions */

	[[nodiscard]] auto isRegion() const -> bool { return !name.empty(); }

	[[nodiscard]] auto toString() const -> std::string
	{
		if (!isRegion())
			return std::format("event {}", first);
		return std::format("region \"{}\" #{} of thread {} [{}..{}]", name, ordinal,
				   first.thread, first, last);
	}
};

enum class EdgeKind : std::uint8_t { po, rf, co, fr, create, join };

auto edgeName(EdgeKind kind) -> const char *
{
	switch (kind) {
	case EdgeKind::po:
		return "po";
	case EdgeKind::rf:
		return "rf";
	case EdgeKind::co:
		return "co";
	case EdgeKind::fr:
		return "fr";
	case EdgeKind::create:
		return "create";
	case EdgeKind::join:
		return "join";
	}
	return "?";
}

/** An edge of the contracted graph, with the events that induce it */
struct Edge {
	unsigned to;
	EdgeKind kind;
	Event from;
	Event toEv;
};

/** The graph with each region contracted to one node */
class ContractedGraph {
public:
	explicit ContractedGraph(const ExecutionGraph &g)
	{
		buildNodes(g);
		buildEdges(g);
	}

	/** Returns the shortest cycle through a region, as its edges */
	[[nodiscard]] auto findRegionCycle() const -> std::optional<std::vector<const Edge *>>
	{
		for (auto n = 0U; n < nodes_.size(); ++n) {
			if (!nodes_[n].isRegion())
				continue;
			if (auto cycle = shortestCycleThrough(n))
				return cycle;
		}
		return std::nullopt;
	}

	[[nodiscard]] auto getNode(unsigned n) const -> const Node & { return nodes_[n]; }
	[[nodiscard]] auto nodeOf(Event e) const -> unsigned { return nodeOf_.at(e); }

private:
	void buildNodes(const ExecutionGraph &g)
	{
		for (auto tid : g.thr_ids()) {
			auto depth = 0U;
			auto ordinal = 0;
			for (const auto &lab : g.po(tid)) {
				if (genmc::isa<InitLabel>(&lab))
					continue;
				/* Nested markers belong to the outermost region */
				if (const auto *bLab = genmc::dyn_cast<MethodBeginLabel>(&lab);
				    bLab && depth++ == 0)
					nodes_.push_back({.first = lab.getPos(),
							  .last = lab.getPos(),
							  .name = bLab->getName(),
							  .ordinal = ++ordinal});
				else if (depth == 0)
					nodes_.push_back(
						{.first = lab.getPos(), .last = lab.getPos()});
				nodes_.back().last = lab.getPos();
				nodeOf_[lab.getPos()] = static_cast<unsigned>(nodes_.size() - 1);
				if (genmc::isa<MethodEndLabel>(&lab) && depth > 0)
					--depth;
			}
		}
		succs_.resize(nodes_.size());
	}

	void addEdge(const EventLabel *from, const EventLabel *to, EdgeKind kind)
	{
		/* The initializer starts every chain: it is on no cycle */
		if (genmc::isa<InitLabel>(from) || genmc::isa<InitLabel>(to))
			return;
		auto src = nodeOf_.at(from->getPos());
		auto dst = nodeOf_.at(to->getPos());
		if (src != dst)
			succs_[src].push_back({.to = dst,
					       .kind = kind,
					       .from = from->getPos(),
					       .toEv = to->getPos()});
	}

	/* Immediate po, co and fr successors suffice: their closure is the one
	 * of the full relations, so the cycles of the contracted graph are too */
	void buildEdges(const ExecutionGraph &g)
	{
		for (const auto &lab : g.labels()) {
			if (const auto *next = g.po_imm_succ(&lab))
				addEdge(&lab, next, EdgeKind::po);
			if (const auto *sLab = genmc::dyn_cast<ThreadStartLabel>(&lab);
			    sLab && sLab->getCreate())
				addEdge(sLab->getCreate(), sLab, EdgeKind::create);
			if (const auto *fLab = genmc::dyn_cast<ThreadFinishLabel>(&lab);
			    fLab && fLab->getParentJoin())
				addEdge(fLab, fLab->getParentJoin(), EdgeKind::join);
			if (const auto *rLab = genmc::dyn_cast<ReadLabel>(&lab)) {
				const auto *wLab = rLab->getRf();
				if (!wLab)
					continue;
				addEdge(wLab, rLab, EdgeKind::rf);
				/* (A plain write outside coherence has no co-successors) */
				if (ExecutionGraph::isOutsideCo(wLab))
					continue;
				if (const auto *frLab = g.fr_imm_succ(rLab))
					addEdge(rLab, frLab, EdgeKind::fr);
			}
			if (const auto *wLab = genmc::dyn_cast<WriteLabel>(&lab);
			    wLab && wLab->isInCo()) {
				if (const auto *coLab = g.co_imm_succ(wLab))
					addEdge(wLab, coLab, EdgeKind::co);
			}
		}
	}

	/** BFS from the successors of N back to N */
	[[nodiscard]] auto shortestCycleThrough(unsigned n) const
		-> std::optional<std::vector<const Edge *>>
	{
		std::vector<const Edge *> via(nodes_.size(), nullptr);
		std::vector<unsigned> parent(nodes_.size(), UINT32_MAX);
		std::deque<unsigned> queue{n};
		parent[n] = n;
		while (!queue.empty()) {
			auto curr = queue.front();
			queue.pop_front();
			for (const auto &e : succs_[curr]) {
				if (e.to == n) {
					std::vector<const Edge *> cycle{&e};
					for (auto m = curr; m != n; m = parent[m])
						cycle.push_back(via[m]);
					std::ranges::reverse(cycle);
					return cycle;
				}
				if (parent[e.to] != UINT32_MAX)
					continue;
				parent[e.to] = curr;
				via[e.to] = &e;
				queue.push_back(e.to);
			}
		}
		return std::nullopt;
	}

	std::vector<Node> nodes_;
	std::unordered_map<Event, unsigned> nodeOf_;
	std::vector<std::vector<Edge>> succs_;
};

} // namespace

auto SerializabilityChecker::check(const ExecutionGraph &g) -> std::optional<SerializabilityError>
{
	ContractedGraph cg(g);
	auto cycle = cg.findRegionCycle();
	if (!cycle)
		return std::nullopt;

	/* The cycle starts at a region: name it and the next node that is one
	 * (or the first other node, when the region is the only one on it) */
	const auto &first = cg.getNode(cg.nodeOf((*cycle).front()->from));
	const auto *second = &cg.getNode((*cycle).front()->to);
	for (const auto *e : *cycle) {
		if (cg.getNode(e->to).isRegion() && &cg.getNode(e->to) != &first) {
			second = &cg.getNode(e->to);
			break;
		}
	}

	std::ostringstream s;
	s << "Atomic regions are not serializable: " << first.toString() << " and "
	  << second->toString() << " are on a cycle of conflicts:\n";
	for (const auto *e : *cycle)
		s << std::format("  {} --{}--> {}\n", e->from, edgeName(e->kind), e->toEv);
	return SerializabilityError{.pos = first.first, .msg = s.str()};
}
