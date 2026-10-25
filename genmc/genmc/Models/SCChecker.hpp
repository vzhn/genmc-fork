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

/*******************************************************************************
 * CAUTION: This file is generated automatically by Kater -- DO NOT EDIT.
 * From kat/sc-genmc.kat, Kater v0.4.0-vzhn.2 (scripts/lkmm/kater-export.sh).
 *******************************************************************************/

#ifndef GENMC_SC_CHECKER_HPP
#define GENMC_SC_CHECKER_HPP

#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Models/ConsistencyChecker.hpp"
#include <algorithm>
#include <concepts>
#include <cstdint>
#include <vector>

// NOLINTBEGIN
class SCChecker : public ConsistencyChecker {

private:
	enum class NodeStatus : uint32_t { unseen = 0, entered = 1, left = 2 };

	struct DFSWorklistEntry {
		DFSWorklistEntry() = default;
		DFSWorklistEntry(uint32_t state, const EventLabel *lab, bool finishing = false)
			: nfaState(state), lab(lab), isFinishing(finishing)
		{}
		uint32_t nfaState;
		const EventLabel *lab;
		bool isFinishing;
	};

	template <std::unsigned_integral T> class NodeStatusVector {
	public:
		NodeStatusVector() = default;
		NodeStatusVector(size_t size) : visited(size), baseStatus(0) {}

		[[nodiscard]] auto getStatus(size_t i) const -> NodeStatus
		{
			auto status = visited[i];
			if (status < baseStatus)
				return NodeStatus::unseen;
			return static_cast<NodeStatus>(status - baseStatus);
		}

		void setStatus(size_t i, NodeStatus s)
		{
			visited[i] = baseStatus + static_cast<T>(s);
		}

		void maybeClearResize(size_t newSize)
		{
			if (baseStatus >= std::numeric_limits<T>::max() - 6) {
				visited.clear();
				baseStatus = 0;
			} else {
				baseStatus += 3;
			}
			visited.resize(newSize);
		}

	private:
		mutable std::vector<T> visited;
		T baseStatus{};
	};

	/* Tarjan's algorithm, for the acyclicity checks: each node keeps its
	 * index, the least index it reaches (its lowlink), the number of
	 * starting nodes on the stack up to it, and the node it was reached
	 * from. A node is `entered` while it is on the stack, and `left` once
	 * its component is complete */
	class NodeSCCVector {
	public:
		struct Node {
			uint32_t status{};
			uint32_t index{};
			uint32_t lowlink{};
			uint32_t starting{};
			Node *parent{};

			/* left is entered + 1 */
			void leave() { ++status; }
		};

		[[nodiscard]] auto getStatus(size_t i) const -> NodeStatus
		{
			auto status = nodes[i].status;
			if (status < baseStatus)
				return NodeStatus::unseen;
			return static_cast<NodeStatus>(status - baseStatus);
		}

		[[nodiscard]] auto get(size_t i) const -> const Node & { return nodes[i]; }

		auto enter(size_t i, uint32_t index, uint32_t starting, Node *parent) -> Node *
		{
			nodes[i] = {baseStatus + static_cast<uint32_t>(NodeStatus::entered), index,
				    index, starting, parent};
			return &nodes[i];
		}

		void maybeClearResize(size_t newSize)
		{
			if (baseStatus >= UINT32_MAX - 6) {
				nodes.clear();
				baseStatus = 0;
			} else {
				baseStatus += 3;
			}
			nodes.resize(newSize);
		}

	private:
		std::vector<Node> nodes;
		uint32_t baseStatus{};
	};

	/* The search for a cycle through a starting node. The nodes on the stack
	 * from any node an edge leads back to are all in the edge's component, so
	 * the edge closes such a cycle iff a starting node is among them. The
	 * stack is kept as the nodes being explored, innermost first through
	 * their parents, and apart, the explored ones that are not the root of
	 * their component */
	class SCCSearch {
	public:
		void reset()
		{
			explored = nullptr;
			pending.clear();
			index = 0;
			starting = 0;
		}

		/* Puts node I of NODES on the stack, to explore it */
		[[gnu::always_inline]] void enter(NodeSCCVector &nodes, size_t i, bool isStarting)
		{
			starting += static_cast<uint32_t>(isStarting);
			explored = nodes.enter(i, index++, starting, explored);
		}

		/* The node explored has an edge to node I of NODES, which is on the
		 * stack: returns whether the edge closes a cycle through a starting node */
		[[gnu::always_inline]] auto reach(const NodeSCCVector &nodes, size_t i,
						  bool isStarting) -> bool
		{
			const auto &n = nodes.get(i);
			if (starting > n.starting || isStarting)
				return true;
			explored->lowlink = std::min(explored->lowlink, n.index);
			return false;
		}

		/* Done exploring the innermost node explored: if it reaches nothing
		 * below it on the stack, its component, it and the nodes above it,
		 * is complete. Its only starting node can be itself, alone: the
		 * search stops at any cycle through one */
		[[gnu::always_inline]] void finish(bool isStarting)
		{
			auto *n = explored;
			explored = n->parent;
			if (n->lowlink < n->index) {
				explored->lowlink = std::min(explored->lowlink, n->lowlink);
				pending.push_back(n);
				return;
			}
			starting -= static_cast<uint32_t>(isStarting);
			n->leave();
			while (!pending.empty() && pending.back()->index > n->index) {
				pending.back()->leave();
				pending.pop_back();
			}
		}

	private:
		NodeSCCVector::Node *explored{}; /* the innermost node being explored */
		std::vector<NodeSCCVector::Node *>
			pending; /* explored, in a component not complete */
		uint32_t index{};
		uint32_t starting{}; /* the starting nodes on the stack */
	};

public:
	SCChecker(const Config *conf) : ConsistencyChecker(conf) {};

protected: // subclasses may refine the generated checks (e.g., hand-written axioms)
	bool isConsistent(const EventLabel *lab) const override;
	bool isConsistent(const ExecutionGraph &g) const override;
	bool isCoherentRelinche(const ExecutionGraph &g) const override;
	std::optional<VerificationError> checkErrors(const EventLabel *lab,
						     const EventLabel *&race) const override;
	std::vector<VerificationError>
	checkWarnings(const EventLabel *lab, const VSet<VerificationError> &reported,
		      std::vector<const EventLabel *> &races) const override;
	std::vector<EventLabel *> getCoherentStores(ReadLabel *rLab) override;
	void filterCoherentRevisits(WriteLabel *sLab, std::vector<ReadLabel *> &ls) override;
	std::vector<EventLabel *> getCoherentPlacings(WriteLabel *sLab) override;
	void updateMMViews(EventLabel *lab) override;
	std::unique_ptr<VectorClock> calculatePrefixView(const EventLabel *lab) const override;
	std::unique_ptr<VectorClock>
	calculateRevisitPrefixView(const EventLabel *lab) const override;
	bool isDepTracking() const override;
	void calculateSaved(EventLabel *lab);
	void calculateViews(EventLabel *lab);
	mutable const EventLabel *cexLab{};
	void recomputeCacheCounters(const ExecutionGraph &g) const override;
	void resetCacheCounters() const override;
	void maybeDecreaseCacheCounters(const EventLabel *lab) const override;
	void maybeIncreaseCacheCounters(const EventLabel *lab) const override;

	mutable NodeStatusVector<uint32_t> visitedCalc72_0;
	mutable NodeStatusVector<uint32_t> visitedCalc72_1;
	mutable NodeStatusVector<uint32_t> visitedCalc72_2;

	bool visitCalc72Iterative(std::vector<DFSWorklistEntry> &worklist, View &calcRes) const;

	View visitCalc72(const EventLabel *lab) const;
	auto getPorfView(const EventLabel *lab) const -> const View & { return lab->view(0); }
	auto getPorfViewIndex() const -> unsigned { return 0; }

	auto checkCalc72(const EventLabel *lab) const;
	mutable NodeStatusVector<uint32_t> visitedCalc73_0;
	mutable NodeStatusVector<uint32_t> visitedCalc73_1;
	mutable NodeStatusVector<uint32_t> visitedCalc73_2;

	bool visitCalc73Iterative(std::vector<DFSWorklistEntry> &worklist, View &calcRes) const;

	View visitCalc73(const EventLabel *lab) const;
	auto getHbViewIndex() const -> unsigned { return 1; }

	auto checkCalc73(const EventLabel *lab) const;
	mutable NodeStatusVector<uint32_t> visitedCalc74_0;
	mutable NodeStatusVector<uint32_t> visitedCalc74_1;
	mutable NodeStatusVector<uint32_t> visitedCalc74_2;

	bool visitCalc74Iterative(std::vector<DFSWorklistEntry> &worklist, View &calcRes) const;

	View visitCalc74(const EventLabel *lab) const;
	auto getHbRelincheView(const EventLabel *lab) const -> const View & { return lab->view(2); }
	auto getHbRelincheViewIndex() const -> unsigned { return 2; }

	auto checkCalc74(const EventLabel *lab) const;
	mutable NodeStatusVector<uint32_t> visitedCoherence_0;
	mutable NodeStatusVector<uint32_t> visitedCoherence_1;
	mutable NodeStatusVector<uint32_t> visitedCoherence_2;
	mutable NodeStatusVector<uint32_t> visitedCoherence_3;
	mutable NodeStatusVector<uint32_t> visitedCoherence_4;
	mutable NodeStatusVector<uint32_t> visitedCoherence_5;
	mutable NodeStatusVector<uint32_t> visitedCoherence_6;

	bool visitCoherenceIterative(std::vector<DFSWorklistEntry> &worklist,
				     const EventLabel *initLab) const;

	bool visitCoherenceRelinche(const ExecutionGraph &g) const;

	mutable NodeSCCVector visitedConsAcyclic1_0;
	mutable SCCSearch visitedConsAcyclic1Search;

	bool visitConsAcyclic1Iterative(std::vector<DFSWorklistEntry> &worklist) const;

	bool visitConsAcyclic1(const EventLabel *lab) const;

	bool visitConsAcyclic1Full(const ExecutionGraph &g) const;

	bool checkUnlessConsAcyclic1([[maybe_unused]] const EventLabel *lab) { return false; }
	bool checkConsAcyclic1(const EventLabel *lab) const;
	bool checkConsAcyclic1(const ExecutionGraph &g) const;
	bool visitError2(const EventLabel *lab) const;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError2_0;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError2_1;

	bool visitLHSUnlessError2Iterative(std::vector<DFSWorklistEntry> &worklist,
					   const View &v) const;

	bool visitUnlessError2(const EventLabel *lab) const;
	bool checkError2(const EventLabel *lab) const;
	bool visitError3(const EventLabel *lab) const;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError3_0;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError3_1;

	bool visitLHSUnlessError3Iterative(std::vector<DFSWorklistEntry> &worklist) const;

	bool visitRHSUnlessError3Iterative(std::vector<DFSWorklistEntry> &worklist) const;

	bool visitUnlessError3(const EventLabel *lab) const;
	bool checkError3(const EventLabel *lab) const;
	bool visitError4(const EventLabel *lab) const;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError4_0;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError4_1;

	bool visitLHSUnlessError4Iterative(std::vector<DFSWorklistEntry> &worklist,
					   const View &v) const;

	bool visitUnlessError4(const EventLabel *lab) const;
	bool checkError4(const EventLabel *lab) const;
	bool visitError5(const EventLabel *lab) const;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError5_0;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError5_1;

	bool visitLHSUnlessError5Iterative(std::vector<DFSWorklistEntry> &worklist) const;

	bool visitRHSUnlessError5Iterative(std::vector<DFSWorklistEntry> &worklist) const;

	bool visitUnlessError5(const EventLabel *lab) const;
	bool checkError5(const EventLabel *lab) const;
	bool visitError6(const EventLabel *lab) const;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError6_0;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError6_1;

	bool visitLHSUnlessError6Iterative(std::vector<DFSWorklistEntry> &worklist,
					   const View &v) const;

	bool visitUnlessError6(const EventLabel *lab) const;
	bool checkError6(const EventLabel *lab) const;
	bool visitError7(const EventLabel *lab) const;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError7_0;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError7_1;

	bool visitLHSUnlessError7Iterative(std::vector<DFSWorklistEntry> &worklist) const;

	bool visitRHSUnlessError7Iterative(std::vector<DFSWorklistEntry> &worklist) const;

	bool visitUnlessError7(const EventLabel *lab) const;
	bool checkError7(const EventLabel *lab) const;
	bool visitError8(const EventLabel *lab) const;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError8_0;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError8_1;

	bool visitLHSUnlessError8Iterative(std::vector<DFSWorklistEntry> &worklist,
					   const View &v) const;

	bool visitUnlessError8(const EventLabel *lab) const;
	bool checkError8(const EventLabel *lab) const;
	bool visitWarning9(const EventLabel *lab) const;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessWarning9_0;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessWarning9_1;

	bool visitLHSUnlessWarning9Iterative(std::vector<DFSWorklistEntry> &worklist,
					     const View &v) const;

	bool visitUnlessWarning9(const EventLabel *lab) const;
	bool checkWarning9(const EventLabel *lab) const;

	void visitPPoRf0(const EventLabel *lab, View &pporf) const;
	void visitPPoRf1(const EventLabel *lab, View &pporf) const;

	View calcPPoRfBefore(const EventLabel *lab) const;

	mutable std::vector<NodeStatus> visitedPPoRf0;
	mutable std::vector<NodeStatus> visitedPPoRf1;
};

// NOLINTEND

#endif /* GENMC_SC_CHECKER_HPP */
