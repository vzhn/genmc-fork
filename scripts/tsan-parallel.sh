#!/bin/bash

# Runs parallel exploration under ThreadSanitizer and fails on any report
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# Needs a genmc built with -DSANITIZE=thread (ctest target "tsan-parallel"
# is registered only for such builds). A data race between exploration
# workers is a bug even when the run happens to succeed, so any report fails.
#
#   GenMC=<build>/bin/genmc scripts/tsan-parallel.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
NTHREADS="${NTHREADS:-4}"
export TSAN_OPTIONS="${TSAN_OPTIONS:-halt_on_error=0 report_signal_unsafe=0}"

# Programs with many backward revisits, so that many graph copies change
# hands between workers
TESTS=(
	"tests/correct/infr/imm-parallel-copy/variants/imm-parallel-copy.c"
)
MODELS=(rc11 imm)

status=0
for t in "${TESTS[@]}"; do
	for m in "${MODELS[@]}"; do
		out=$("${GenMC}" "-${m}" -disable-estimation -disable-mm-detector \
			"-nthreads=${NTHREADS}" -- "${DIR}/../${t}" 2>&1)
		rc=$?
		races=$(echo "${out}" | grep -c 'WARNING: ThreadSanitizer')
		if [[ ${rc} -ne 0 || ${races} -ne 0 ]]; then
			echo "FAIL ${t} (${m}): exit ${rc}, ${races} ThreadSanitizer report(s)"
			echo "${out}" | grep -A12 'WARNING: ThreadSanitizer' | head -60
			status=1
		else
			echo "ok   ${t} (${m})"
		fi
	done
done
exit ${status}
