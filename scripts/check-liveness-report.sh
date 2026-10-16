#!/bin/bash

# Checks that a liveness violation is reported (not an abort) with default flags
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# The wrong-test driver runs every test with -disable-estimation. This runs
# liveness violations whose report replays to a non-atomic access with
# default flags (estimation on), several times per model: the report must
# name the stuck thread, and GenMC must not abort. ctest target
# "liveness-report".
#
#   GenMC=<build>/bin/genmc scripts/check-liveness-report.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
TESTS=(
	"${DIR}/../tests/wrong/liveness/await-stuck-join/variants/await-stuck-join.c"
	"${DIR}/../tests/wrong/liveness/spin-na-stuck/variants/spin-na-stuck.c"
)
RUNS="${RUNS:-3}"

status=0
for t in "${TESTS[@]}"; do
	for m in sc rc11 imm; do
		ok=1
		for i in $(seq 1 "${RUNS}"); do
			out=$("${GenMC}" "-${m}" -disable-mm-detector -check-liveness -- "${t}" 2>&1)
			if ! echo "${out}" | grep -qE 'Non-terminating (wait|spinloop): thread' ||
				echo "${out}" | grep -q 'INTERNAL FAILURE'; then
				echo "FAIL $(basename "${t}") ${m} run ${i}: no liveness report"
				echo "${out}" | grep -E 'INTERNAL|Internal check|Error|No errors' | head -3
				ok=0
				status=1
			fi
		done
		[[ ${ok} -eq 1 ]] && echo "ok   $(basename "${t}") ${m} (${RUNS} runs)"
	done
done
exit ${status}
