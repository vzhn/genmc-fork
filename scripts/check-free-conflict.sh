#!/bin/bash

# Checks that a double free and an access after free name the free they
# conflict with, under every model and under -exhaustive -imm
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# The checkers named the conflicting event of these errors through a pointer
# only other checks set: no event, a wrong one, or (under -exhaustive -imm,
# which checks again on a copy of the graph) one of a destroyed copy, which
# crashed GenMC on list-channel-double-free in about half the runs (#199).
# The test suite runs the memory tests under RC11 only. ctest target
# "free-conflict".
#
#   GenMC=<build>/bin/genmc scripts/check-free-conflict.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
TESTS="${DIR}/../tests/wrong/memory"

status=0
check() { # <test dir> <expected line> <flags>...
	local t="$1" want="$2"; shift 2
	local out
	out=$("${GenMC}" -disable-estimation -disable-mm-detector "$@" -- \
		"${TESTS}/${t}/variants/${t}.c" 2>&1)
	local rc=$?
	if [ ${rc} -eq 42 ] && echo "${out}" | grep -qF "${want}"; then
		echo "ok   ${t} $*"
	else
		echo "FAIL ${t} $* (rc ${rc}): expected \"${want}\""
		echo "${out}" | grep -E -m2 'Error|conflicts'
		status=1
	fi
}

for flags in -sc -tso -ra -rc11 -imm "-imm -exhaustive"; do
	check double-free-conflict "Event (2, 3) conflicts with event (1, 3)" ${flags}
	check access-freed-conflict "Event (0, 6) conflicts with event (1, 2)" ${flags}
done
for i in 1 2 3 4; do
	check list-channel-double-free "Event (0, 8) conflicts with event (1, 20)" -imm -exhaustive
done
exit ${status}
