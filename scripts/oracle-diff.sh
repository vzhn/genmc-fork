#!/bin/bash

# Compares GenMC's default exploration with the -exhaustive reference.
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT
#
# Usage: oracle-diff.sh [options] <genmc> <test-dir>...
#
# A test dir is a tests/correct/<category> or tests/wrong/<category> directory
# (every test in it runs), or one test directory (with variants/). For each
# args.<model>.mo.in line (or once, without one) and each variant, the test
# runs twice: in the default mode with every reduction that -exhaustive
# disables turned off (-disable-sr -disable-ipr -disable-bam -disable-helper;
# the expected files may assume those reductions, so the comparison is not
# with them), and with -exhaustive. A test disagrees when the two runs differ
# in the number of complete executions (the deduplicated number under
# -exhaustive) or in the verdict (no errors, or an error). Two runs that both
# report an error agree: the exploration stops at the first error, so the
# executions counted before it, and which of several errors comes first,
# depend on the order. Both runs use -disable-estimation and
# -disable-mm-detector, like the test suite.
#
# Options:
#   -m <models>   comma-separated models (default: rc11,imm,sc)
#   -t <seconds>  timeout per run (default: 120)
#   -c <N>        skip the -exhaustive run when the default run explored more
#                 than N executions (default: 20000)
#   -j <N>        parallel runs (default: 1)
#   -f <flags>    extra flags for both runs (e.g., -unroll=2)
#   -v            print every result, not only disagreements and timeouts
#
# Output: one line per run ("ok", "MISMATCH", "TIMEOUT", "SKIP", "FAIL"), then
# a summary. The exit status is 1 if any run disagreed, 2 if a run timed out
# or failed to run (but none disagreed), 0 otherwise. Every disagreement is
# either a bug in the reference (then fix the reference) or a bug in the
# exploration (then an issue and a regression test).

set -u

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

MODELS="rc11,imm,sc"
TIMEOUT=120
CAP=20000
JOBS=1
EXTRA=""
VERBOSE=""

while getopts "m:t:c:j:f:v" opt; do
    case "$opt" in
	m) MODELS="$OPTARG" ;;
	t) TIMEOUT="$OPTARG" ;;
	c) CAP="$OPTARG" ;;
	j) JOBS="$OPTARG" ;;
	f) EXTRA="$OPTARG" ;;
	v) VERBOSE=1 ;;
	*) echo "usage: $0 [-m models] [-t secs] [-c cap] [-j jobs] [-f flags] [-v] <genmc> <test-dir>..." >&2
	   exit 2 ;;
    esac
done
shift $((OPTIND - 1))

if [ $# -lt 2 ]; then
    echo "usage: $0 [-m models] [-t secs] [-c cap] [-j jobs] [-f flags] [-v] <genmc> <test-dir>..." >&2
    exit 2
fi

GenMC="$(readlink -f "$1")"; shift
if [ ! -x "${GenMC}" ]; then
    echo "not an executable: ${GenMC}" >&2
    exit 2
fi

export GenMC TIMEOUT CAP EXTRA VERBOSE

# The reductions -exhaustive turns off, so that the default run explores the
# same executions (the differences left are the exploration's own)
DEFAULT_FLAGS="-disable-sr -disable-ipr -disable-bam -disable-helper"
ORACLE_FLAGS="-exhaustive"
export DEFAULT_FLAGS ORACLE_FLAGS

# run_one <model> <test-dir> <genmc-args> <clang-args> <variant>
# Prints one result line.
run_one() {
    local model="$1" tdir="$2" gargs="$3" cargs="$4" variant="$5"
    local name
    name="$(basename "$(dirname "${tdir}")")/$(basename "${tdir}")/$(basename "${variant}")"
    local tag="${model} ${name}"
    [ -n "${gargs}" ] && tag="${tag} [${gargs}]"

    # The test's own flags, minus the reductions -exhaustive turns off (it
    # would not be a comparison) and the worker count (the reference runs alone)
    gargs=$(echo " ${gargs} " | sed 's/ -blind-rmw-reduction / /g; s/ -nthreads=[0-9]* / /g; s/^ *//; s/ *$//')
    local base="${GenMC} -${model} -disable-mm-detector -disable-estimation ${EXTRA} ${gargs}"
    local out1 out2 rc1 rc2
    out1=$(timeout "${TIMEOUT}" ${base} ${DEFAULT_FLAGS} -- ${cargs} "${variant}" 2>&1)
    rc1=$?
    if [ ${rc1} -eq 124 ]; then
	echo "TIMEOUT ${tag}: default run"
	return
    fi
    local n1 v1
    n1=$(echo "${out1}" | awk '/complete executions explored/ { print $6 }')
    v1=$(verdict "${out1}")
    if [ -z "${n1}" ] && [ "${v1}" = "none" ]; then
	echo "FAIL ${tag}: default run gave no result (rc ${rc1}): $(echo "${out1}" | tail -n 1)"
	return
    fi
    if [ -n "${n1}" ] && [ "${n1}" -gt "${CAP}" ]; then
	echo "SKIP ${tag}: default=${n1} above the cap"
	return
    fi

    out2=$(timeout "${TIMEOUT}" ${base} ${ORACLE_FLAGS} -- ${cargs} "${variant}" 2>&1)
    rc2=$?
    if [ ${rc2} -eq 124 ]; then
	echo "TIMEOUT ${tag}: exhaustive run (default=${n1:-?} ${v1})"
	return
    fi
    local n2 v2
    n2=$(echo "${out2}" | awk '/complete executions explored/ { print $6 }')
    v2=$(verdict "${out2}")
    if [ -z "${n2}" ] && [ "${v2}" = "none" ]; then
	echo "FAIL ${tag}: exhaustive run gave no result (rc ${rc2}): $(echo "${out2}" | tail -n 1)"
	return
    fi
    local raw
    raw=$(echo "${out2}" | awk '/^Exhaustive:/ { print $2 }')
    # An error stops the exploration: the executions counted before it, and
    # which of several errors is met first, depend on the order, so two runs
    # that both report an error agree
    if [ "${v1}" != "none" ] && [ "${v2}" != "none" ]; then
	if [ -n "${VERBOSE}" ]; then
	    echo "ok ${tag}: error in both (default=${n1:-?} ${v1}, exhaustive=${n2:-?} ${v2})"
	fi
	return
    fi
    if [ "${n1}" != "${n2}" ] || [ "${v1}" != "${v2}" ]; then
	echo "MISMATCH ${tag}: default=${n1:-?} ${v1}, exhaustive=${n2:-?} ${v2} (raw ${raw:-?})"
	return
    fi
    if [ -n "${VERBOSE}" ]; then
	echo "ok ${tag}: ${n1:-?} ${v1} (raw ${raw:-?})"
    fi
}

# The verdict of a run: "none" without errors, else the error's first line
verdict() {
    if echo "$1" | grep -q "No errors were detected"; then
	echo "none"
	return
    fi
    local err
    err=$(echo "$1" | grep -m 1 "^Error:\|^Warning:" | tr -s ' ' | cut -c1-60)
    if [ -z "${err}" ]; then
	echo "none"
    else
	echo "${err}" | tr ' ' '_'
    fi
}
export -f run_one verdict

# Lists the runs of one test directory under one model, one per line:
# model TAB dir TAB genmc-args TAB clang-args TAB variant
list_runs() {
    local model="$1" tdir="$2"
    [ -d "${tdir}/variants" ] || return
    local variants=()
    for v in "${tdir}"/variants/*.c "${tdir}"/variants/*.cpp "${tdir}"/variants/*.ll; do
	[ -e "${v}" ] && variants+=("${v}")
    done
    [ ${#variants[@]} -gt 0 ] || return
    # The suite runs a test under a model only with an expected file for it;
    # tests/wrong keeps its genmc.<model>.*.in files instead
    local argsfile="${tdir}/args.${model}.mo.in"
    if [ -f "${tdir}/expected.${model}.mo.in" ] || ls "${tdir}"/genmc."${model}".*.in >/dev/null 2>&1 ||
	   [ -f "${tdir}/expected-error.in" ]; then
	:
    else
	return
    fi
    if [ -f "${argsfile}" ]; then
	while IFS= read -r line; do
	    local gargs cargs
	    gargs=$(echo "${line}" | cut -f1 -d'|' | sed 's/^ *//;s/ *$//')
	    cargs=$(echo "${line}" | cut -f2 -s -d'|' | sed 's/^ *//;s/ *$//')
	    for v in "${variants[@]}"; do
		printf '%s\t%s\t%s\t%s\t%s\n' "${model}" "${tdir}" "${gargs}" "${cargs}" "${v}"
	    done
	done < "${argsfile}"
    else
	# tests/wrong: the per-model flags live in genmc.<model>.{wb,mo}.in
	local gargs=""
	for f in "${tdir}/genmc.${model}.wb.in" "${tdir}/genmc.${model}.mo.in"; do
	    [ -f "${f}" ] && gargs=$(head -n 1 "${f}" | sed 's/^ *//;s/ *$//') && break
	done
	for v in "${variants[@]}"; do
	    printf '%s\t%s\t%s\t%s\t%s\n' "${model}" "${tdir}" "${gargs}" "" "${v}"
	done
    fi
}

runs=$(mktemp)
trap 'rm -f "${runs}"' EXIT
for tdir in "$@"; do
    tdir="${tdir%/}"
    IFS=',' read -ra models <<< "${MODELS}"
    for model in "${models[@]}"; do
	if [ -d "${tdir}/variants" ]; then
	    list_runs "${model}" "${tdir}"
	else
	    for sub in "${tdir}"/*/; do
		list_runs "${model}" "${sub%/}"
	    done
	fi
    done
done >> "${runs}"

total=$(wc -l < "${runs}")
echo "oracle-diff: ${total} runs, models ${MODELS}, timeout ${TIMEOUT}s, cap ${CAP}, ${JOBS} jobs"

results=$(mktemp)
trap 'rm -f "${runs}" "${results}"' EXIT
tr '\t\n' '\0\0' < "${runs}" |
    xargs -0 -n 5 -P "${JOBS}" bash -c 'run_one "$0" "$1" "$2" "$3" "$4"' | tee "${results}"

mismatches=$(grep -c '^MISMATCH' "${results}")
timeouts=$(grep -c '^TIMEOUT' "${results}")
fails=$(grep -c '^FAIL' "${results}")
skips=$(grep -c '^SKIP' "${results}")
agreed=$((total - mismatches - timeouts - fails - skips))
echo "oracle-diff summary: ${total} runs, ${agreed} agree, ${mismatches} mismatches, ${timeouts} timeouts, ${fails} failed, ${skips} skipped (cap)"

if [ "${mismatches}" -gt 0 ]; then
    exit 1
elif [ "${timeouts}" -gt 0 ] || [ "${fails}" -gt 0 ]; then
    exit 2
fi
exit 0
