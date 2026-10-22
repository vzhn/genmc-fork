#!/bin/bash

# Runs the correct/ tests under -sc, which driver.sh's default MODELS leave
# out, so that the expected.sc.mo.in and blocked.sc.mo.in files do not go
# stale unnoticed. One driver.sh run per category (CORRECT_DIRS takes a single
# directory), all in parallel; a test without an expected.sc.mo.in is skipped,
# as under any model. The wrong/ tests have no genmc.sc.*.in files (their
# per-model flags, e.g. -check-liveness or -unroll) and do not run. ctest
# target "sc-driver".
#
#   GenMC=<build>/bin/genmc scripts/sc-driver.sh
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
export GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"

# driver.sh's default correct/ categories
CATEGORIES=(infr litmus saver ipr sr liveness synthetic data-structures classic gobench lkmm)

empty=$(mktemp -d)
logs=$(mktemp -d)
trap 'rm -rf "${empty}" "${logs}"' EXIT

pids=()
for category in "${CATEGORIES[@]}"; do
	MODELS=sc CORRECT_DIRS="${DIR}/../tests/correct/${category}" WRONG_DIRS="${empty}" \
	      "${DIR}/driver.sh" --fast --debug > "${logs}/${category}.log" 2>&1 &
	pids+=($!)
done

status=0
for i in "${!CATEGORIES[@]}"; do
	category="${CATEGORIES[$i]}"
	if wait "${pids[$i]}"; then
		echo "ok   ${category}"
	else
		echo "FAIL ${category}"
		cat "${logs}/${category}.log"
		status=1
	fi
done
exit ${status}
