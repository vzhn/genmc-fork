#!/bin/bash

# Checks when -lkmm warns about unordered writes
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# Under -lkmm, a store with several coherence placings is no write-write race:
# the placings do not respect hb there. Writes ordered by a lock or by a
# release and an acquire must not be reported; unordered marked writes must.
# ctest target "lkmm-ww-warning".
#
#   GenMC=<build>/bin/genmc scripts/check-lkmm-ww-warning.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
TESTS="${DIR}/../tests/correct/lkmm"
WARN='Unordered writes'

status=0
for t in ww-lock-ordered ww-mp-ordered; do
	out=$("${GenMC}" -lkmm -disable-estimation -- "${TESTS}/${t}/variants/${t}.c" 2>&1)
	if echo "${out}" | grep -q "${WARN}"; then
		echo "FAIL ${t}: warned about ordered writes"; echo "${out}" | tail -5; status=1
	else
		echo "ok   ${t}: no warning"
	fi
done

out=$("${GenMC}" -lkmm -disable-estimation -- "${TESTS}/ww-unordered/variants/ww-unordered.c" 2>&1)
if echo "${out}" | grep -q "${WARN}"; then
	echo "ok   ww-unordered: warned"
else
	echo "FAIL ww-unordered: no warning"; echo "${out}" | tail -5; status=1
fi
exit ${status}
