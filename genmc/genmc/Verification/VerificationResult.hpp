#ifndef GENMC_VERIFICATION_RESULT_HPP
#define GENMC_VERIFICATION_RESULT_HPP

#include "genmc/ADT/IndexedMap.hpp"
#include "genmc/ADT/VSet.hpp"
#include "genmc/Verification/Relinche/LinearizabilityChecker.hpp"
#include "genmc/Verification/Relinche/Specification.hpp"
#include "genmc/Support/VerificationError.hpp"

#include <memory>
#include <string>
#include <utility>

struct VerificationResult {
	std::optional<VerificationError> status; /**< Whether the verification
				    completed successfully */
	unsigned explored{};			 /**< Number of complete executions explored */
	unsigned exploredBlocked{};		 /**< Number of blocked executions explored */
	unsigned spinBlocked{};	   /**< Blocked executions with a thread in a spinloop or await */
	unsigned boundExceeding{}; /**< Number of bound-exceeding executions explored */
	unsigned relocationReruns{}; /**< Runs repeated after moving a thread's kept events */
	unsigned relocationsLost{};  /**< Runs dropped: a kept event would be skipped */
	unsigned keptCheckReruns{};  /**< Runs repeated with eager checks of events before kept ones */
	unsigned exploredDuplicates{}; /**< -exhaustive: complete executions seen again (not counted
					  in explored) */
	unsigned graphsVisited{};      /**< -exhaustive: distinct graphs produced by revisits */
	unsigned revisitsRepeated{}; /**< -exhaustive: revisits skipped, their graph seen before */
	unsigned exploredInconsistent{}; /**< Executions not counted: under IMM, inconsistent
					    with its full ar (or, -exhaustive, when checked
					    anew) */
	long double estimationMean{};	  /**< The mean of estimations */
	long double estimationVariance{}; /**< The (biased) variance of the estimations */
	bool inferredSymmetryError{}; /**< Whether the error is unordered writes, an error
					only because of symmetries GenMC inferred */
	bool iprError{}; /**< Whether the error is unordered writes, an error only because
			    of in-place revisiting */
#ifdef ENABLE_GENMC_DEBUG
	unsigned exploredMoot{};	       /**< Number of moot executions _encountered_ */
	unsigned duplicates{};		       /**< Number of duplicate executions explored */
	genmc::IndexedMap<int> exploredBounds; /**< Number of complete executions not
			       exceeding each bound */
#endif
	std::string message;				 /**< A message to be printed */
	VSet<VerificationError> warnings;		 /**< The warnings encountered */
	std::unique_ptr<Specification> specification;	 /**< Spec collected (if any) */
	LinearizabilityChecker::Result relincheResult{}; /**< Spec analysis result */

	VerificationResult() = default;

	// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
	auto operator+=(VerificationResult &&other) -> VerificationResult &
	{
		/* Propagate latest error */
		if (other.status.has_value()) {
			status = other.status;
			inferredSymmetryError = other.inferredSymmetryError;
			iprError = other.iprError;
		}
		message += other.message;
		explored += other.explored;
		exploredBlocked += other.exploredBlocked;
		spinBlocked += other.spinBlocked;
		boundExceeding += other.boundExceeding;
		relocationReruns += other.relocationReruns;
		relocationsLost += other.relocationsLost;
		keptCheckReruns += other.keptCheckReruns;
		exploredDuplicates += other.exploredDuplicates;
		graphsVisited += other.graphsVisited;
		revisitsRepeated += other.revisitsRepeated;
		exploredInconsistent += other.exploredInconsistent;
		estimationMean += other.estimationMean;
		estimationVariance += other.estimationVariance;
#ifdef ENABLE_GENMC_DEBUG
		exploredMoot += other.exploredMoot;
		/* Bound-blocked executions are calculated at the end */
		exploredBounds.grow(other.exploredBounds.size() - 1);
		for (auto i = 0U; i < other.exploredBounds.size(); i++)
			exploredBounds[i] += other.exploredBounds[i];
		duplicates += other.duplicates;
#endif
		warnings.insert(other.warnings);
		if (other.specification)
			specification->merge(
				std::move(*other.specification)); // FIXME other is const lvalue
		relincheResult += std::move(other.relincheResult);
		return *this;
	}
};

#endif /* GENMC_VERIFICATION_RESULT_HPP */
