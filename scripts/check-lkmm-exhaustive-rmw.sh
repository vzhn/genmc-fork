#!/bin/bash

# Checks that -lkmm (exhaustive by default) keeps an RMW's write with its read
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# A backward revisit kept the write part of a fetch_add, read by a kept
# load of its thread, without the fetch_add's read; the graph was later cut
# between the two, and a forward revisit of the read, now a hole, crashed
# in revisitRead() (issue #223). The revisit now keeps the read with the
# write. The program (tests/correct/infr/imm-exhaustive-kept-deps-1070, an
# IMM test) must finish under -lkmm, and every execution it counts must be
# consistent (-check-invariants, I5). (Its count still depends on the
# schedule, see #228, so it is not checked.) ctest target
# "lkmm-exhaustive-rmw".
#
#   GenMC=<build>/bin/genmc scripts/check-lkmm-exhaustive-rmw.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
prog="${DIR}/../tests/correct/infr/imm-exhaustive-kept-deps-1070/variants/imm-exhaustive-kept-deps-1070.c"

out=$("${GenMC}" -lkmm -check-invariants -disable-estimation -- "${prog}" < /dev/null 2>&1)
rc=$?
if [[ ${rc} -eq 0 ]] && echo "${out}" | grep -q 'No errors were detected'; then
	echo "ok   $(echo "${out}" | grep 'complete executions explored')"
	exit 0
fi
echo "FAIL -lkmm on imm-exhaustive-kept-deps-1070: status ${rc}"
echo "${out}" | grep -E 'Invariant|INTERNAL|Segmentation' | head -3
exit 1
