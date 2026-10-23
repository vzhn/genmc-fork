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
 * From kat/lkmm-genmc.kat, Kater 914a4ed (scripts/lkmm/kater-export.sh).
 *******************************************************************************/

#ifndef GENMC_LKMM_CHECKER_HPP
#define GENMC_LKMM_CHECKER_HPP

#include "genmc/Execution/EventLabel.hpp"
#include "genmc/Models/ConsistencyChecker.hpp"
#include <concepts>
#include <cstdint>
#include <vector>

// NOLINTBEGIN
class LKMMChecker : public ConsistencyChecker {

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

	class NodeVisitStatusVector {
	private:
		struct NodeVisitStatus {
			NodeVisitStatus() = default;
			NodeVisitStatus(uint32_t count, uint32_t status)
				: count(count), status(status)
			{}
			uint32_t count{};
			uint32_t status{};
		};

	public:
		NodeVisitStatusVector() = default;
		NodeVisitStatusVector(size_t size)
			: visited(size), baseStatus(0), baseCount(0), currIterationMaxCount(0)
		{}

		[[nodiscard]] auto getStatus(size_t i) const -> NodeStatus
		{
			auto status = visited[i].status;
			if (status < baseStatus)
				return NodeStatus::unseen;
			return static_cast<NodeStatus>(status - baseStatus);
		}

		void setStatus(size_t i, NodeStatus s)
		{
			visited[i].status = baseStatus + static_cast<uint32_t>(s);
		}

		[[nodiscard]] auto getCount(size_t i) const -> uint32_t
		{
			auto count = visited[i].count;
			if (count < baseCount)
				return 0;
			return count - baseCount;
		}

		auto setCountIncr(size_t i, uint32_t count)
		{
			const uint32_t newCount = baseCount + count;
			visited[i].count = newCount;
			currIterationMaxCount = std::max(currIterationMaxCount, newCount);
		}

		void setIncr(size_t i, uint32_t count, NodeStatus status)
		{
			setCountIncr(i, count);
			setStatus(i, status);
		}

		void set(size_t i, uint32_t count, NodeStatus status)
		{
			visited[i].count = baseCount + count;
			setStatus(i, status);
		}

		void maybeClearResize(size_t newSize)
		{
			if (baseStatus >= UINT32_MAX - 6 ||
			    currIterationMaxCount > UINT32_MAX / 2) {
				visited.clear();
				baseStatus = 0;
				baseCount = 0;
			} else {
				baseStatus += 3;
				baseCount = currIterationMaxCount;
			}
			currIterationMaxCount = 0;
			visited.resize(newSize);
		}

	private:
		mutable std::vector<NodeVisitStatus> visited;
		uint32_t baseStatus{};
		uint32_t baseCount{};
		uint32_t currIterationMaxCount{};
	};

public:
	LKMMChecker(const Config *conf) : ConsistencyChecker(conf) {};

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

	mutable NodeStatusVector<uint32_t> visitedCalc125_0;
	mutable NodeStatusVector<uint32_t> visitedCalc125_1;
	mutable NodeStatusVector<uint32_t> visitedCalc125_2;

	bool visitCalc125Iterative(std::vector<DFSWorklistEntry> &worklist, View &calcRes) const;

	View visitCalc125(const EventLabel *lab) const;
	auto getCohView(const EventLabel *lab) const -> const View & { return lab->view(0); }
	auto getCohViewIndex() const -> unsigned { return 0; }

	auto checkCalc125(const EventLabel *lab) const;
	mutable NodeStatusVector<uint32_t> visitedCalc126_0;
	mutable NodeStatusVector<uint32_t> visitedCalc126_1;
	mutable NodeStatusVector<uint32_t> visitedCalc126_2;

	bool visitCalc126Iterative(std::vector<DFSWorklistEntry> &worklist, View &calcRes) const;

	View visitCalc126(const EventLabel *lab) const;
	auto getHbViewIndex() const -> unsigned { return 1; }

	auto checkCalc126(const EventLabel *lab) const;
	mutable NodeStatusVector<uint32_t> visitedCoherence_0;
	mutable NodeStatusVector<uint32_t> visitedCoherence_1;
	mutable NodeStatusVector<uint32_t> visitedCoherence_2;
	mutable NodeStatusVector<uint32_t> visitedCoherence_3;
	mutable NodeStatusVector<uint32_t> visitedCoherence_4;

	bool visitCoherenceIterative(std::vector<DFSWorklistEntry> &worklist,
				     const EventLabel *initLab) const;

	bool visitCoherenceRelinche(const ExecutionGraph &g) const;

	mutable NodeVisitStatusVector visitedConsAcyclic1_0;
	mutable NodeVisitStatusVector visitedConsAcyclic1_1;
	mutable NodeVisitStatusVector visitedConsAcyclic1_2;
	mutable NodeVisitStatusVector visitedConsAcyclic1_3;
	mutable NodeVisitStatusVector visitedConsAcyclic1_4;
	mutable NodeVisitStatusVector visitedConsAcyclic1_5;
	mutable NodeVisitStatusVector visitedConsAcyclic1_6;
	mutable NodeVisitStatusVector visitedConsAcyclic1_7;
	mutable NodeVisitStatusVector visitedConsAcyclic1_8;
	mutable NodeVisitStatusVector visitedConsAcyclic1_9;
	mutable NodeVisitStatusVector visitedConsAcyclic1_10;
	mutable NodeVisitStatusVector visitedConsAcyclic1_11;
	mutable NodeVisitStatusVector visitedConsAcyclic1_12;
	mutable NodeVisitStatusVector visitedConsAcyclic1_13;
	mutable NodeVisitStatusVector visitedConsAcyclic1_14;
	mutable NodeVisitStatusVector visitedConsAcyclic1_15;
	mutable NodeVisitStatusVector visitedConsAcyclic1_16;
	mutable NodeVisitStatusVector visitedConsAcyclic1_17;
	mutable NodeVisitStatusVector visitedConsAcyclic1_18;
	mutable NodeVisitStatusVector visitedConsAcyclic1_19;
	mutable NodeVisitStatusVector visitedConsAcyclic1_20;
	mutable NodeVisitStatusVector visitedConsAcyclic1_21;
	mutable NodeVisitStatusVector visitedConsAcyclic1_22;
	mutable NodeVisitStatusVector visitedConsAcyclic1_23;
	mutable NodeVisitStatusVector visitedConsAcyclic1_24;
	mutable uint32_t visitedConsAcyclic1Accepting;

	bool visitConsAcyclic1Iterative(std::vector<DFSWorklistEntry> &worklist) const;

	bool visitConsAcyclic1(const EventLabel *lab) const;

	bool visitConsAcyclic1Full(const ExecutionGraph &g) const;

	bool checkUnlessConsAcyclic1([[maybe_unused]] const EventLabel *lab) { return false; }
	bool checkConsAcyclic1(const EventLabel *lab) const;
	bool checkConsAcyclic1(const ExecutionGraph &g) const;
	mutable NodeVisitStatusVector visitedConsAcyclic2_0;
	mutable NodeVisitStatusVector visitedConsAcyclic2_1;
	mutable NodeVisitStatusVector visitedConsAcyclic2_2;
	mutable NodeVisitStatusVector visitedConsAcyclic2_3;
	mutable NodeVisitStatusVector visitedConsAcyclic2_4;
	mutable NodeVisitStatusVector visitedConsAcyclic2_5;
	mutable NodeVisitStatusVector visitedConsAcyclic2_6;
	mutable NodeVisitStatusVector visitedConsAcyclic2_7;
	mutable NodeVisitStatusVector visitedConsAcyclic2_8;
	mutable NodeVisitStatusVector visitedConsAcyclic2_9;
	mutable NodeVisitStatusVector visitedConsAcyclic2_10;
	mutable NodeVisitStatusVector visitedConsAcyclic2_11;
	mutable NodeVisitStatusVector visitedConsAcyclic2_12;
	mutable NodeVisitStatusVector visitedConsAcyclic2_13;
	mutable NodeVisitStatusVector visitedConsAcyclic2_14;
	mutable NodeVisitStatusVector visitedConsAcyclic2_15;
	mutable NodeVisitStatusVector visitedConsAcyclic2_16;
	mutable NodeVisitStatusVector visitedConsAcyclic2_17;
	mutable NodeVisitStatusVector visitedConsAcyclic2_18;
	mutable NodeVisitStatusVector visitedConsAcyclic2_19;
	mutable NodeVisitStatusVector visitedConsAcyclic2_20;
	mutable NodeVisitStatusVector visitedConsAcyclic2_21;
	mutable NodeVisitStatusVector visitedConsAcyclic2_22;
	mutable NodeVisitStatusVector visitedConsAcyclic2_23;
	mutable NodeVisitStatusVector visitedConsAcyclic2_24;
	mutable NodeVisitStatusVector visitedConsAcyclic2_25;
	mutable NodeVisitStatusVector visitedConsAcyclic2_26;
	mutable NodeVisitStatusVector visitedConsAcyclic2_27;
	mutable NodeVisitStatusVector visitedConsAcyclic2_28;
	mutable NodeVisitStatusVector visitedConsAcyclic2_29;
	mutable NodeVisitStatusVector visitedConsAcyclic2_30;
	mutable NodeVisitStatusVector visitedConsAcyclic2_31;
	mutable NodeVisitStatusVector visitedConsAcyclic2_32;
	mutable NodeVisitStatusVector visitedConsAcyclic2_33;
	mutable NodeVisitStatusVector visitedConsAcyclic2_34;
	mutable NodeVisitStatusVector visitedConsAcyclic2_35;
	mutable NodeVisitStatusVector visitedConsAcyclic2_36;
	mutable NodeVisitStatusVector visitedConsAcyclic2_37;
	mutable NodeVisitStatusVector visitedConsAcyclic2_38;
	mutable NodeVisitStatusVector visitedConsAcyclic2_39;
	mutable NodeVisitStatusVector visitedConsAcyclic2_40;
	mutable NodeVisitStatusVector visitedConsAcyclic2_41;
	mutable NodeVisitStatusVector visitedConsAcyclic2_42;
	mutable NodeVisitStatusVector visitedConsAcyclic2_43;
	mutable NodeVisitStatusVector visitedConsAcyclic2_44;
	mutable NodeVisitStatusVector visitedConsAcyclic2_45;
	mutable NodeVisitStatusVector visitedConsAcyclic2_46;
	mutable uint32_t visitedConsAcyclic2Accepting;

	bool visitConsAcyclic2Iterative(std::vector<DFSWorklistEntry> &worklist) const;

	bool visitConsAcyclic2(const EventLabel *lab) const;

	bool visitConsAcyclic2Full(const ExecutionGraph &g) const;

	bool checkUnlessConsAcyclic2([[maybe_unused]] const EventLabel *lab) { return false; }
	bool checkConsAcyclic2(const EventLabel *lab) const;
	bool checkConsAcyclic2(const ExecutionGraph &g) const;
	bool visitError3(const EventLabel *lab) const;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError3_0;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError3_1;

	bool visitLHSUnlessError3Iterative(std::vector<DFSWorklistEntry> &worklist,
					   const View &v) const;

	bool visitUnlessError3(const EventLabel *lab) const;
	bool checkError3(const EventLabel *lab) const;
	bool visitError4(const EventLabel *lab) const;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError4_0;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError4_1;

	bool visitLHSUnlessError4Iterative(std::vector<DFSWorklistEntry> &worklist) const;

	bool visitRHSUnlessError4Iterative(std::vector<DFSWorklistEntry> &worklist) const;

	bool visitUnlessError4(const EventLabel *lab) const;
	bool checkError4(const EventLabel *lab) const;
	bool visitError5(const EventLabel *lab) const;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError5_0;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError5_1;

	bool visitLHSUnlessError5Iterative(std::vector<DFSWorklistEntry> &worklist,
					   const View &v) const;

	bool visitUnlessError5(const EventLabel *lab) const;
	bool checkError5(const EventLabel *lab) const;
	bool visitError6(const EventLabel *lab) const;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError6_0;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessError6_1;

	bool visitLHSUnlessError6Iterative(std::vector<DFSWorklistEntry> &worklist) const;

	bool visitRHSUnlessError6Iterative(std::vector<DFSWorklistEntry> &worklist) const;

	bool visitUnlessError6(const EventLabel *lab) const;
	bool checkError6(const EventLabel *lab) const;
	bool visitWarning7(const EventLabel *lab) const;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessWarning7_0;
	mutable NodeStatusVector<uint32_t> visitedLHSUnlessWarning7_1;

	bool visitLHSUnlessWarning7Iterative(std::vector<DFSWorklistEntry> &worklist,
					     const View &v) const;

	bool visitUnlessWarning7(const EventLabel *lab) const;
	bool checkWarning7(const EventLabel *lab) const;

	void visitPPoRf0(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf1(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf2(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf3(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf4(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf5(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf6(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf7(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf8(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf9(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf10(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf11(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf12(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf13(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf14(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf15(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf16(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf17(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf18(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf19(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf20(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf21(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf22(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf23(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf24(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf25(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf26(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf27(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf28(const EventLabel *lab, DepView &pporf) const;
	void visitPPoRf29(const EventLabel *lab, DepView &pporf) const;

	DepView calcPPoRfBefore(const EventLabel *lab) const;

	mutable std::vector<NodeStatus> visitedPPoRf0;
	mutable std::vector<NodeStatus> visitedPPoRf1;
	mutable std::vector<NodeStatus> visitedPPoRf2;
	mutable std::vector<NodeStatus> visitedPPoRf3;
	mutable std::vector<NodeStatus> visitedPPoRf4;
	mutable std::vector<NodeStatus> visitedPPoRf5;
	mutable std::vector<NodeStatus> visitedPPoRf6;
	mutable std::vector<NodeStatus> visitedPPoRf7;
	mutable std::vector<NodeStatus> visitedPPoRf8;
	mutable std::vector<NodeStatus> visitedPPoRf9;
	mutable std::vector<NodeStatus> visitedPPoRf10;
	mutable std::vector<NodeStatus> visitedPPoRf11;
	mutable std::vector<NodeStatus> visitedPPoRf12;
	mutable std::vector<NodeStatus> visitedPPoRf13;
	mutable std::vector<NodeStatus> visitedPPoRf14;
	mutable std::vector<NodeStatus> visitedPPoRf15;
	mutable std::vector<NodeStatus> visitedPPoRf16;
	mutable std::vector<NodeStatus> visitedPPoRf17;
	mutable std::vector<NodeStatus> visitedPPoRf18;
	mutable std::vector<NodeStatus> visitedPPoRf19;
	mutable std::vector<NodeStatus> visitedPPoRf20;
	mutable std::vector<NodeStatus> visitedPPoRf21;
	mutable std::vector<NodeStatus> visitedPPoRf22;
	mutable std::vector<NodeStatus> visitedPPoRf23;
	mutable std::vector<NodeStatus> visitedPPoRf24;
	mutable std::vector<NodeStatus> visitedPPoRf25;
	mutable std::vector<NodeStatus> visitedPPoRf26;
	mutable std::vector<NodeStatus> visitedPPoRf27;
	mutable std::vector<NodeStatus> visitedPPoRf28;
	mutable std::vector<NodeStatus> visitedPPoRf29;

	void visitRPPoRf0(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf1(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf2(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf3(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf4(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf5(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf6(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf7(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf8(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf9(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf10(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf11(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf12(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf13(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf14(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf15(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf16(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf17(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf18(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf19(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf20(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf21(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf22(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf23(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf24(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf25(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf26(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf27(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf28(const EventLabel *lab, DepView &pporf) const;
	void visitRPPoRf29(const EventLabel *lab, DepView &pporf) const;

	DepView calcRPPoRfBefore(const EventLabel *lab) const;

	mutable std::vector<NodeStatus> visitedRPPoRf0;
	mutable std::vector<NodeStatus> visitedRPPoRf1;
	mutable std::vector<NodeStatus> visitedRPPoRf2;
	mutable std::vector<NodeStatus> visitedRPPoRf3;
	mutable std::vector<NodeStatus> visitedRPPoRf4;
	mutable std::vector<NodeStatus> visitedRPPoRf5;
	mutable std::vector<NodeStatus> visitedRPPoRf6;
	mutable std::vector<NodeStatus> visitedRPPoRf7;
	mutable std::vector<NodeStatus> visitedRPPoRf8;
	mutable std::vector<NodeStatus> visitedRPPoRf9;
	mutable std::vector<NodeStatus> visitedRPPoRf10;
	mutable std::vector<NodeStatus> visitedRPPoRf11;
	mutable std::vector<NodeStatus> visitedRPPoRf12;
	mutable std::vector<NodeStatus> visitedRPPoRf13;
	mutable std::vector<NodeStatus> visitedRPPoRf14;
	mutable std::vector<NodeStatus> visitedRPPoRf15;
	mutable std::vector<NodeStatus> visitedRPPoRf16;
	mutable std::vector<NodeStatus> visitedRPPoRf17;
	mutable std::vector<NodeStatus> visitedRPPoRf18;
	mutable std::vector<NodeStatus> visitedRPPoRf19;
	mutable std::vector<NodeStatus> visitedRPPoRf20;
	mutable std::vector<NodeStatus> visitedRPPoRf21;
	mutable std::vector<NodeStatus> visitedRPPoRf22;
	mutable std::vector<NodeStatus> visitedRPPoRf23;
	mutable std::vector<NodeStatus> visitedRPPoRf24;
	mutable std::vector<NodeStatus> visitedRPPoRf25;
	mutable std::vector<NodeStatus> visitedRPPoRf26;
	mutable std::vector<NodeStatus> visitedRPPoRf27;
	mutable std::vector<NodeStatus> visitedRPPoRf28;
	mutable std::vector<NodeStatus> visitedRPPoRf29;
};

// NOLINTEND

#endif /* GENMC_LKMM_CHECKER_HPP */
