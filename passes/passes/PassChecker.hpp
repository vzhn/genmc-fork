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

#ifndef GENMC_PASS_CHECKER_HPP
#define GENMC_PASS_CHECKER_HPP

#include "passes/ModuleInfo.hpp"

#include <llvm/ADT/Any.h>
#include <llvm/IR/PassInstrumentation.h>
#include <llvm/IR/PassManager.h>

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

/*
 * Debugging aids for the transformation passes (-output-llvm-passes,
 * -verify-pass-analyses).
 *
 * With an output directory, each pass that changes the module leaves the
 * module before and after it there (NNNN-<pass>-<function>.src.ll and
 * .tgt.ll), and a line in passes.txt: "NNNN <pass> <function> <src> <tgt>",
 * with "-" as the function of a module pass. Alive2 can then check each pair
 * for refinement (scripts/check-passes.sh). In both modules, atomic accesses
 * are calls (Alive2 has no atomics). The passes that block executions on
 * purpose are checked against what they claim: after SpinAssumePass, the
 * target is the module with each blocking marker replaced by its claim, and
 * the load annotations get a pair of their own (LoadAnnotations).
 *
 * Under a model with dependencies (IMM, LKMM), a pass that keeps the memory
 * layout also gets a "+deps" pair: both modules report the address, data and
 * control dependencies of each access, as GenMC's interpreter computes them,
 * so that Alive2 checks the pass keeps them.
 *
 * With analysis verification, the dominator tree a pass declares preserved
 * is checked against a fresh one: the loop pass adaptor declares the tree
 * preserved whatever a loop pass did to the CFG.
 */
class PassChecker {
public:
	/* Which annotations EliminateAnnotationsPass keeps as attributes
	 * (the helper, confirmation and final-write options) */
	struct AnnotationKinds;

	/* Which dependencies GenMC tracks: none, control dependencies to the
	 * end of the thread (IMM), or each until its branch's join (LKMM) */
	enum class DepMode : std::uint8_t { None, Unscoped, Scoped };

	PassChecker(std::string outDir, bool verifyAnalyses, bool annotHelper, bool annotConf,
		    bool annotFinal, DepMode depMode, llvm::FunctionAnalysisManager &fam)
		: outDir(std::move(outDir)), verifyAnalyses(verifyAnalyses),
		  annotHelper(annotHelper), annotConf(annotConf), annotFinal(annotFinal),
		  depMode(depMode), fam(fam)
	{}

	void registerCallbacks(llvm::PassInstrumentationCallbacks &PIC);

	/* With an output directory, the pair of the final module and its load
	 * annotations' claims */
	void writeAnnotationObligations(
		const llvm::Module &mod,
		const AnnotationInfo<llvm::Instruction *, llvm::Value *> &annots);

private:
	[[nodiscard]] auto kinds() const -> AnnotationKinds;
	void before(llvm::StringRef passID, llvm::Any IR);
	void after(llvm::StringRef passID, llvm::Any IR, const llvm::PreservedAnalyses &PA);
	void verifyDominatorTree(llvm::StringRef passID, llvm::Function &fun,
				 const llvm::PreservedAnalyses &PA);

	/* Writes a pair of modules and its line in passes.txt; returns the
	 * pair's path without its .src.ll/.tgt.ll suffix */
	auto record(const std::string &pass, const std::string &fun, const std::string &src,
		    const std::string &tgt) -> std::string;

	std::string outDir;
	bool verifyAnalyses;
	bool annotHelper;
	bool annotConf;
	bool annotFinal;
	DepMode depMode;
	llvm::FunctionAnalysisManager &fam;

	/* The module's text before each running pass */
	std::vector<std::string> snapshots;

	/* The loop passes run on a function since its last check */
	std::map<const llvm::Function *, std::set<std::string>> loopPasses;

	unsigned count = 0;
};

#endif /* GENMC_PASS_CHECKER_HPP */
