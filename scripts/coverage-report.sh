#!/bin/bash
#
# Source-based coverage of GenMC (a build configured with ENABLE_COVERAGE=ON
# and clang): runs the unit and driver tests, with --full also the fast
# regression suite and the integration ctests (the check scripts, Relinche,
# the pass checks with alive-tv, parallel and randomized runs) through the
# instrumented genmc, and writes to
# <build>/coverage:
#
#   html/index.html   the annotated sources
#   report.txt        lines, regions, functions and branches per file
#   summary.txt       the same per directory (one row per layer and group)
#   coverage.lcov     lcov export, for tools that read it
#
# Usage: scripts/coverage-report.sh <build> [--full]
#   (the build's `coverage` and `coverage-full` targets run it)
# LLVM_PROFDATA and LLVM_COV must match the compiler (the targets set them).
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

set -u
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
ROOT="$(cd "${DIR}/.." && pwd)"
BUILD="$(cd "${1:?usage: coverage-report.sh <build> [--full]}" && pwd)"
FULL="${2:-}"
PROFDATA="${LLVM_PROFDATA:-llvm-profdata}"
COV="${LLVM_COV:-llvm-cov}"
OUT="${BUILD}/coverage"

rm -rf "${OUT}"
mkdir -p "${OUT}/profraw"
# One profile per binary and pool slot: the many genmc runs of the suite
# merge into at most 8 files instead of one each
export LLVM_PROFILE_FILE="${OUT}/profraw/%8m.profraw"

status=0
objects=()
for t in unit_tests driver_tests; do
	[[ -x "${BUILD}/bin/${t}" ]] || continue
	objects+=("${BUILD}/bin/${t}")
	echo "--- ${t}"
	"${BUILD}/bin/${t}" > "${OUT}/${t}.log" 2>&1 || { echo "FAIL: ${t} (see ${OUT}/${t}.log)"; status=1; }
done
if [[ -x "${BUILD}/bin/genmc" ]]; then
	objects+=("${BUILD}/bin/genmc")
	if [[ "${FULL}" == "--full" ]]; then
		echo "--- fast regression suite"
		# (driver.sh runs each genmc under GENMC_MEMORY_LIMIT, see #188)
		GenMC="${BUILD}/bin/genmc" "${ROOT}/scripts/driver.sh" --fast > "${OUT}/driver.log" 2>&1 ||
			{ echo "FAIL: driver.sh --fast (see ${OUT}/driver.log)"; status=1; }
		echo "--- integration ctests"
		# (not fast-driver: the suite above; not the ThreadSanitizer run.
		# The parallel and randomized runs go without ctest's timeouts,
		# which the instrumented genmc would exceed)
		GenMC="${BUILD}/bin/genmc" ctest --test-dir "${BUILD}" -L integration -j "$(nproc)" \
			-E '^(fast-driver|tsan-parallel|run-parallel|randomize-driver)$' \
			> "${OUT}/ctest.log" 2>&1 ||
			{ echo "FAIL: integration ctests (see ${OUT}/ctest.log)"; status=1; }
		(cd "${ROOT}" &&
			GenMC="${BUILD}/bin/genmc" scripts/run-parallel.sh &&
			GenMC="${BUILD}/bin/genmc" iterations=1 scripts/randomize-driver.sh) \
			> "${OUT}/parallel.log" 2>&1 ||
			{ echo "FAIL: parallel and randomized runs (see ${OUT}/parallel.log)"; status=1; }
	fi
fi
[[ ${#objects[@]} -gt 0 ]] || { echo "no test binaries in ${BUILD}/bin"; exit 1; }

compgen -G "${OUT}/profraw/*.profraw" > /dev/null || { echo "no profiles written: is the build instrumented?"; exit 1; }
"${PROFDATA}" merge -sparse "${OUT}"/profraw/*.profraw -o "${OUT}/coverage.profdata" || exit 1
rm -rf "${OUT}/profraw"

# GenMC's own sources only: not the tests, the build, or system headers
ignore='(^/usr/|/tests/|/build[^/]*/|/_deps/)'
args=(-instr-profile="${OUT}/coverage.profdata" -ignore-filename-regex="${ignore}" "${objects[0]}")
for o in "${objects[@]:1}"; do
	args+=(-object "${o}")
done

"${COV}" report "${args[@]}" > "${OUT}/report.txt" || exit 1
"${COV}" show -format=html -show-line-counts-or-regions -output-dir="${OUT}/html" "${args[@]}" || exit 1
"${COV}" export -format=lcov "${args[@]}" > "${OUT}/coverage.lcov" || exit 1
"${COV}" export -format=text -summary-only "${args[@]}" |
	python3 "${DIR}/coverage-summary.py" "${ROOT}" > "${OUT}/summary.txt" || exit 1

cat "${OUT}/summary.txt"
echo "HTML report: ${OUT}/html/index.html"
exit ${status}
