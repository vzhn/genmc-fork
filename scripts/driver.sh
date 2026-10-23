#!/bin/bash

# Driver script for running tests with GenMC.
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Get binary's full path
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
GENMCFLAGS="${GENMCFLAGS:-}"

# A memory limit for everything this script runs (KiB of virtual memory, or
# "unlimited"): a test that runs away, e.g. an unbounded loop under a model
# without the test's bounding arguments (#188), then fails instead of
# exhausting the machine. A soft limit, so a stricter one already set stays.
# Sanitizer builds reserve terabytes of address space: they run without it.
GENMC_MEMORY_LIMIT="${GENMC_MEMORY_LIMIT:-16000000}"
if grep -qaE '__(asan|tsan|msan)_init' "${GenMC}" 2>/dev/null; then
	GENMC_MEMORY_LIMIT=unlimited
fi
ulimit -S -v "${GENMC_MEMORY_LIMIT}" ||
	{ echo "driver.sh: cannot set the memory limit to ${GENMC_MEMORY_LIMIT} (GENMC_MEMORY_LIMIT)" >&2; exit 1; }
# The models to test (space-separated in the MODELS environment variable, e.g. MODELS="sc rc11")
MODELS=(${MODELS:-rc11 imm lkmm})

CONFIG_FILE="$(dirname "$(dirname "${GenMC}")")/include/genmc/config.h"

# Set the directories for testing
if [ -z ${CORRECT_DIRS+x} ]
then
    CORRECT_DIRS=(infr litmus saver ipr sr liveness synthetic data-structures classic gobench lkmm)
    CORRECT_DIRS=("${CORRECT_DIRS[@]/#/${SCRIPT_DIR}/../tests/correct/}" )
fi
if [ -z ${WRONG_DIRS+x} ]
then
    WRONG_DIRS=(safety liveness infr racy memory locking barriers helper)
    WRONG_DIRS=("${WRONG_DIRS[@]/#/${SCRIPT_DIR}/../tests/wrong/}" )
fi

source "${SCRIPT_DIR}/terminal.sh"

# We need to get the LLVM version for this particular configuration
LLVM_VERSION=`cat ${CONFIG_FILE} | awk '/LLVM_VERSION/ {gsub(/"/, "", $3); print $3}'`

# test whether arrays are supported
arrtest[0]='test' ||
    (echo 'Failure: arrays not supported in this version of bash.' && exit 2)

# parse the options until we see --
while [[ $# -gt 0 ]]; do
    case "$1" in
	-f|--fast)
	    fastrun=1
	    shift
	    ;;
	-d|--debug)
	    debug_mode=1
	    shift
	    ;;
        -v|--verbose)
            verbosity_level="$2"
            shift 2
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "Programming error"
            exit 3
            ;;
    esac
done

# assumes that the result variable is appropriate set by the child script
initialize_results() {
    total_time=0
    result=""
    header_printed=""
}

# assumes that the (sourced) child script sets the runtime variable
increase_total_time() {
    total_time=`echo "scale=2; ${total_time}+${runtime}" | bc -l`
}

print_results() {
    if test -n "$result"
    then
	echo ''; printline
	echo '!!! ' UNEXPECTED TESTING RESULTS ' !!!' Total time: "${total_time}"
	printline
	exit 1
    else
	echo ''; printline
	echo '--- ' Testing proceeded as expected. Total time: "${total_time}"
	printline
	exit 0
    fi
}

# Do initializations
initialize_results

# First, run the test cases in the correct/ directory
for model in "${MODELS[@]}"
do
    for testdir in "${CORRECT_DIRS[@]}"
    do
	cat="${testdir##*/}"

	# skip certain combinations
	if [[ "${cat}" == "helper" && "${GENMCFLAGS}" =~ "policy=arbitrary" ]]
	then
	    continue
	fi
	if [[ "${model}" == "imm" && "${cat}" == "sr" ]]
	then
	    continue
	fi
	# (under lkmm, a test runs iff it has an expected.lkmm.mo.in: runcorrect.sh)

	check_blocked="" && [[ "${cat}" == "saver" || "${cat}" == "helper" ]] &&
	    [[ (! "${GENMCFLAGS}" =~ "policy=arbitrary") ]] && [[ "${model}" != "imm" ]] &&
	    check_blocked="yes"
	if [[ "${cat}" == "ipr" && "${model}" != "imm" ]]
	then
	    check_blocked="yes"
	fi
	source "${SCRIPT_DIR}/runcorrect.sh" # the env variables for runcorrect.sh are set
	increase_total_time
    done
done

# Then, run the testcases in the wrong/ directory
header_printed=""

for model in "${MODELS[@]}"
do
    for testdir in "${WRONG_DIRS[@]}"
    do
	cat="${testdir##*/}"
	# under IMM, only run safety and liveness tests
	if test "${model}" = "imm" -a "${cat}" != "safety" -a "${cat}" != "liveness"
	then
	    continue
	fi
	# (under LKMM, a test runs iff it has a genmc.lkmm.<coherence>.in: runwrong.sh)
	if [[ "${cat}" == "liveness" || "${cat}" == "helper" ]]
	then
	    coherence="mo"
	else
	    coherence="wb"
	fi
	suppress_diff=1
	# if test "${cat}" = "memory" -o "${cat}" = "fs"
	# then
	#     suppress_diff=1
	# fi
	source "${SCRIPT_DIR}/runwrong.sh"
	increase_total_time
    done
done

# Print results
print_results
