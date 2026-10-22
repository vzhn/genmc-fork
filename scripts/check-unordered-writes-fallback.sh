#!/bin/bash

# Checks that GenMC starts over without SR or IPR whenever they meet
# unordered writes, whatever the schedule
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# Unordered writes are an error under symmetry reduction once the graph has
# symmetric threads, and under in-place revisiting at the location of an
# annotated read; GenMC then starts over without the optimization (#130).
# The decision must not depend on the order the exploration adds events in
# (schedule seeds), on an earlier warning about other writes, or on
# -disable-race-detection. ctest target "unordered-writes-fallback".
#
#   GenMC=<build>/bin/genmc scripts/check-unordered-writes-fallback.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
TESTS="${DIR}/../tests/correct"
WAINC="${TESTS}/synthetic/wainc/variants/wainc0.c"
SYMM="${TESTS}/sr/sym-readers-after-ww/variants/sym-readers-after-ww.c"
IPR="${TESTS}/infr/ipr-ww-rmw-after-store/variants/ipr-ww-rmw-after-store.c"
SEEDS="1 2 3 7 2809334225"

status=0
explored() { "${GenMC}" -rc11 -disable-estimation "$@" 2>&1 | awk '/explored/ { print $6 }'; }
expect() { # <name> <expected> <genmc args...>
	local name="$1" exp="$2"
	shift 2
	local n
	n=$(explored "$@")
	if [[ "${n}" == "${exp}" ]]; then
		echo "ok   ${name}: ${n}"
	else
		echo "FAIL ${name}: ${n:-no count}, expected ${exp}"; status=1
	fi
}

# Without SR: (N+1)! executions; with it, N+1
for s in ${SEEDS}; do
	expect "wainc N=2, seed ${s}" 6 -schedule-policy=arbitrary -schedule-seed=${s} -- -DN=2 "${WAINC}"
	expect "wainc N=3, seed ${s}" 24 -schedule-policy=arbitrary -schedule-seed=${s} -- -DN=3 "${WAINC}"
	expect "sym-readers-after-ww, seed ${s}" 18 -schedule-policy=arbitrary -schedule-seed=${s} -- "${SYMM}"
done
expect "wainc N=2, -disable-race-detection" 6 -disable-race-detection -- -DN=2 "${WAINC}"
expect "wainc N=3, -disable-race-detection" 24 -disable-race-detection -- -DN=3 "${WAINC}"

for s in default ${SEEDS}; do
	args=() && [[ "${s}" != default ]] && args=(-schedule-policy=arbitrary -schedule-seed=${s})
	out=$("${GenMC}" -rc11 -disable-estimation "${args[@]}" -- "${IPR}" 2>&1)
	if echo "${out}" | grep -q 'In-place revisiting does not support'; then
		echo "ok   ipr-ww-rmw-after-store, ${s}: started over without IPR"
	else
		echo "FAIL ipr-ww-rmw-after-store, ${s}: kept IPR"; echo "${out}" | tail -3; status=1
	fi
done
exit ${status}
