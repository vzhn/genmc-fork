#!/bin/bash
#
# Differential test for regenerated consistency checkers: runs two GenMC
# binaries (a reference build and one with regenerated checkers) on the same
# programs under each memory model and reports every program whose outcome
# differs. The outcome is the verdict (no error, or the first error line) and
# the numbers of complete and blocked executions.
#
# Usage: compare-checkers.sh <ref-genmc> <new-genmc> <file-or-dir>...
#
# Directories are searched for test variants (*/variants/*.c). Environment:
# MODELS (default: "sc tso ra rc11 imm"), GENMC_TIMEOUT (default: 20s),
# JOBS (default: number of CPUs), GENMCFLAGS (extra flags for both binaries).

set -uo pipefail

if test $# -lt 3; then
	echo "usage: $0 <ref-genmc> <new-genmc> <file-or-dir>..." >&2
	exit 2
fi

export REF=$(realpath "$1") NEW=$(realpath "$2")
shift 2
export MODELS="${MODELS:-sc tso ra rc11 imm}"
export GENMC_TIMEOUT="${GENMC_TIMEOUT:-20s}"
export GENMCFLAGS="${GENMCFLAGS:-}"
JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN)}"

# Prints the outcome of one run on one line
outcome() {
	local out rc
	out=$(timeout "${GENMC_TIMEOUT}" "$1" "-$2" -disable-mm-detector \
		-disable-estimation ${GENMCFLAGS} -- "$3" 2>&1)
	rc=$?
	if test "${rc}" -eq 124; then
		echo "timeout"
		return
	fi
	local err complete blocked
	err=$(echo "${out}" | grep -m1 -E '^Error|^[A-Z][a-z-]* violation|race|^Assertion' |
		sed 's/ (.*//')
	complete=$(echo "${out}" | sed -n 's/^Number of complete executions explored: //p')
	blocked=$(echo "${out}" | sed -n 's/^Number of blocked executions seen: //p')
	echo "rc=${rc} err=${err:-none} complete=${complete:--} blocked=${blocked:-0}"
}
export -f outcome

compare_one() {
	local f=$1 m a b
	for m in ${MODELS}; do
		a=$(outcome "${REF}" "${m}" "${f}")
		b=$(outcome "${NEW}" "${m}" "${f}")
		if test "${a}" = "${b}"; then
			echo "same ${m} ${f} ${a}"
		else
			echo "DIFF ${m} ${f} ref[${a}] new[${b}]"
		fi
	done
}
export -f compare_one

for arg in "$@"; do
	if test -d "${arg}"; then
		find "${arg}" -path '*/variants/*.c' | sort
	else
		echo "${arg}"
	fi
done | xargs -r -d '\n' -n 1 -P "${JOBS}" bash -c 'compare_one "$0"' |
	sort -k3 | tee /dev/stderr |
	awk '{ n[$1]++ } END { printf "same: %d, differ: %d\n", n["same"], n["DIFF"]; exit n["DIFF"] > 0 }'
