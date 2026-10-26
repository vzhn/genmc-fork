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
# With -p, the two runs are of two models instead, a stronger and a weaker one
# (e.g., -p sc:rc11): every execution consistent under the stronger is
# consistent under the weaker, so the weaker must explore at least as many.
# The tests are those the suite runs under the stronger model, with its
# args.<model>.mo.in lines, and both runs are in the default mode with the
# reductions above turned off (-lkmm explores exhaustively anyway). A test
# disagrees when the weaker model explores fewer executions, or when the
# stronger reports an error and the weaker does not; an error under the weaker
# model alone is an execution the stronger forbids, and agrees.
#
# Options:
#   -m <models>   comma-separated models (default: rc11,imm,sc)
#   -p <pairs>    comma-separated <stronger>:<weaker> model pairs, in place of
#                 -m and -exhaustive
#   -k <file>     known disagreements (default: oracle-diff-known.txt next to
#                 this script)
#   -t <seconds>  timeout per run (default: 120)
#   -c <N>        skip the second run when the first explored more than N
#                 executions (default: 20000)
#   -j <N>        parallel runs (default: 1)
#   -f <flags>    extra flags for both runs (e.g., -unroll=2)
#   -v            print every result, not only disagreements and timeouts
#
# Output: one line per run ("ok", "MISMATCH", "KNOWN", "TIMEOUT", "SKIP",
# "FAIL"), then a summary. The exit status is 1 if any run disagreed and is
# not listed in the known file, or if a listed test ran and agreed (remove its
# line); 2 if a run of a test not listed timed out or failed to run (but none
# disagreed), 0 otherwise. Every disagreement is either a bug in the reference
# (then fix the reference) or a bug in the exploration (then an issue and a
# regression test); under -p, also a pair of models that are not ordered on
# that program (then a line in the known file that says why).

set -u

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

MODELS="rc11,imm,sc"
PAIRS=""
KNOWN="${DIR}/oracle-diff-known.txt"
TIMEOUT=120
CAP=20000
JOBS=1
EXTRA=""
VERBOSE=""
USAGE="usage: $0 [-m models | -p pairs] [-k known] [-t secs] [-c cap] [-j jobs] [-f flags] [-v] <genmc> <test-dir>..."

while getopts "m:p:k:t:c:j:f:v" opt; do
    case "$opt" in
	m) MODELS="$OPTARG" ;;
	p) PAIRS="$OPTARG" ;;
	k) KNOWN="$OPTARG" ;;
	t) TIMEOUT="$OPTARG" ;;
	c) CAP="$OPTARG" ;;
	j) JOBS="$OPTARG" ;;
	f) EXTRA="$OPTARG" ;;
	v) VERBOSE=1 ;;
	*) echo "${USAGE}" >&2
	   exit 2 ;;
    esac
done
shift $((OPTIND - 1))

if [ $# -lt 2 ]; then
    echo "${USAGE}" >&2
    exit 2
fi

GenMC="$(readlink -f "$1")"; shift
if [ ! -x "${GenMC}" ]; then
    echo "not an executable: ${GenMC}" >&2
    exit 2
fi

export GenMC TIMEOUT CAP EXTRA VERBOSE KNOWN

# The reductions -exhaustive turns off, so that the default run explores the
# same executions (the differences left are the exploration's own)
DEFAULT_FLAGS="-disable-sr -disable-ipr -disable-bam -disable-helper"
ORACLE_FLAGS="-exhaustive"
export DEFAULT_FLAGS ORACLE_FLAGS

# run_one <model> <test-dir> <genmc-args> <clang-args> <variant>
# Prints one result line. <model> is a model, run in the default mode and with
# -exhaustive, or a <stronger>:<weaker> pair (-p).
run_one() {
    local model="$1" tdir="$2" gargs="$3" cargs="$4" variant="$5"
    local name
    name="$(basename "$(dirname "${tdir}")")/$(basename "${tdir}")/$(basename "${variant}")"
    local tag="${model} ${name}"
    [ -n "${gargs}" ] && tag="${tag} [${gargs}]"

    local m1="${model%%:*}" m2="${model#*:}" f1="${DEFAULT_FLAGS}" f2="${ORACLE_FLAGS}"
    local l1=default l2=exhaustive
    if [ "${m1}" != "${model}" ]; then
	f2="${DEFAULT_FLAGS}"
	l1="${m1}"
	l2="${m2}"
    fi

    # The test's own flags, minus the reductions -exhaustive turns off (it
    # would not be a comparison) and the worker count (the reference runs alone)
    gargs=$(echo " ${gargs} " | sed 's/ -blind-rmw-reduction / /g; s/ -nthreads=[0-9]* / /g; s/^ *//; s/ *$//')
    local base="-disable-mm-detector -disable-estimation ${EXTRA} ${gargs}"
    local out1 out2 rc1 rc2
    out1=$(timeout "${TIMEOUT}" ${GenMC} -${m1} ${base} ${f1} -- ${cargs} "${variant}" 2>&1)
    rc1=$?
    if [ ${rc1} -eq 124 ]; then
	echo "TIMEOUT ${tag}: ${l1} run"
	return
    fi
    local n1 v1
    n1=$(echo "${out1}" | awk '/complete executions explored/ { print $6 }')
    v1=$(verdict "${out1}")
    if [ -z "${n1}" ] && [ "${v1}" = "none" ]; then
	echo "FAIL ${tag}: ${l1} run gave no result (rc ${rc1}): $(echo "${out1}" | tail -n 1)"
	return
    fi
    if [ -n "${n1}" ] && [ "${n1}" -gt "${CAP}" ]; then
	echo "SKIP ${tag}: ${l1}=${n1} above the cap"
	return
    fi

    out2=$(timeout "${TIMEOUT}" ${GenMC} -${m2} ${base} ${f2} -- ${cargs} "${variant}" 2>&1)
    rc2=$?
    if [ ${rc2} -eq 124 ]; then
	echo "TIMEOUT ${tag}: ${l2} run (${l1}=${n1:-?} ${v1})"
	return
    fi
    local n2 v2
    n2=$(echo "${out2}" | awk '/complete executions explored/ { print $6 }')
    v2=$(verdict "${out2}")
    if [ -z "${n2}" ] && [ "${v2}" = "none" ]; then
	echo "FAIL ${tag}: ${l2} run gave no result (rc ${rc2}): $(echo "${out2}" | tail -n 1)"
	return
    fi
    local raw
    raw=$(echo "${out2}" | awk '/^Exhaustive:/ { print $2 }')
    local result="${l1}=${n1:-?} ${v1}, ${l2}=${n2:-?} ${v2}"
    # An error stops the exploration: the executions counted before it, and
    # which of several errors is met first, depend on the order, so two runs
    # that both report an error agree
    if [ "${v1}" != "none" ] && [ "${v2}" != "none" ]; then
	if [ -n "${VERBOSE}" ]; then
	    echo "ok ${tag}: error in both (${result})"
	fi
	return
    fi
    local differ=""
    if [ "${m1}" = "${model}" ]; then
	[ "${n1}" != "${n2}" ] || [ "${v1}" != "${v2}" ] && differ=1
    else
	# The weaker model may reach an error the stronger forbids
	[ "${v1}" != "none" ] || { [ "${v2}" = "none" ] && [ "${n2}" -lt "${n1}" ]; } && differ=1
    fi
    if [ -n "${differ}" ]; then
	if is_known "${model}" "${tdir}"; then
	    echo "KNOWN ${tag}: ${result} (raw ${raw:-?})"
	else
	    echo "MISMATCH ${tag}: ${result} (raw ${raw:-?})"
	fi
	return
    fi
    if [ -n "${VERBOSE}" ]; then
	echo "ok ${tag}: ${result} (raw ${raw:-?})"
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
# The key of a test in the known file: model (or pair), category/test
test_key() {
    echo "$1 $(basename "$(dirname "$2")")/$(basename "$2")"
}

# Whether the known file lists <model> <test-dir>
is_known() {
    local key
    key=$(test_key "$1" "$2")
    [ -f "${KNOWN}" ] && grep -vE '^\s*(#|$)' "${KNOWN}" | awk '{ print $1 " " $2 }' | grep -qxF "${key}"
}
export -f run_one verdict test_key is_known

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
	while IFS= read -r line || [[ -n "${line}" ]]; do
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
    # A pair runs the tests of its stronger model
    IFS=',' read -ra models <<< "${PAIRS:-${MODELS}}"
    for model in "${models[@]}"; do
	strong="${model%%:*}"
	if [ -d "${tdir}/variants" ]; then
	    list_runs "${strong}" "${tdir}"
	else
	    for sub in "${tdir}"/*/; do
		list_runs "${strong}" "${sub%/}"
	    done
	fi | sed "s/^${strong}\t/${model}\t/"
    done
done >> "${runs}"

total=$(wc -l < "${runs}")
what="models ${MODELS}"
[ -n "${PAIRS}" ] && what="pairs ${PAIRS}"
echo "oracle-diff: ${total} runs, ${what}, timeout ${TIMEOUT}s, cap ${CAP}, ${JOBS} jobs"

results=$(mktemp)
trap 'rm -f "${runs}" "${results}"' EXIT
tr '\t\n' '\0\0' < "${runs}" |
    xargs -0 -n 5 -P "${JOBS}" bash -c 'run_one "$0" "$1" "$2" "$3" "$4"' | tee "${results}"

mismatches=$(grep -c '^MISMATCH' "${results}")
knowns=$(grep -c '^KNOWN' "${results}")
timeouts=$(grep -c '^TIMEOUT' "${results}")
fails=$(grep -c '^FAIL' "${results}")
skips=$(grep -c '^SKIP' "${results}")
agreed=$((total - mismatches - knowns - timeouts - fails - skips))
echo "oracle-diff summary: ${total} runs, ${agreed} agree, ${mismatches} mismatches, ${knowns} known, ${timeouts} timeouts, ${fails} failed, ${skips} skipped (cap)"

# A listed test that ran to the end on every line and agreed: its entry is
# stale (a run still known to differ, or one without a result, keeps it)
result_keys() {
    grep "^$1 " "${results}" | awk '{ k = $3; sub(/:$/, "", k); sub(/\/[^\/]*$/, "", k); print $2 " " k }' | sort -u
}
ran=$(cut -f1,2 "${runs}" | while IFS=$'\t' read -r m t; do test_key "${m}" "${t}"; done | sort -u)
listed=$([ -f "${KNOWN}" ] && grep -vE '^\s*(#|$)' "${KNOWN}" | awk '{ print $1 " " $2 }' | sort -u)
kept=$(for k in KNOWN TIMEOUT FAIL SKIP; do result_keys "${k}"; done | sort -u)
stale=$(comm -12 <(echo "${listed}") <(echo "${ran}") | comm -23 - <(echo "${kept}") | sed '/^$/d')
if [ -n "${stale}" ]; then
    echo "oracle-diff: these tests in $(basename "${KNOWN}") now agree; remove their lines:"
    echo "${stale}" | sed 's/^/  /'
fi

# A listed test may time out or fail to run too (that is what its line is for)
noresult=$(for k in TIMEOUT FAIL; do result_keys "${k}"; done | sort -u | comm -23 - <(echo "${listed}") | sed '/^$/d')

if [ "${mismatches}" -gt 0 ] || [ -n "${stale}" ]; then
    exit 1
elif [ -n "${noresult}" ]; then
    exit 2
fi
exit 0
