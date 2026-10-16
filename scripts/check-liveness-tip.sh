#!/bin/bash

# Checks that a stuck wait is pointed out when -check-liveness is off
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# A wait that never finishes must not pass silently: without -check-liveness
# GenMC prints a tip naming the flag, and with it reports the violation (and
# no tip). ctest target "liveness-tip".
#
#   GenMC=<build>/bin/genmc scripts/check-liveness-tip.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
TEST="${DIR}/../tests/wrong/liveness/await-stuck/variants/await-stuck0.c"
TIP='Use -check-liveness to report the waits that can never finish'

status=0
out=$("${GenMC}" -disable-estimation -- "${TEST}" 2>&1)
if echo "${out}" | grep -q "${TIP}"; then
	echo "ok   tip printed without -check-liveness"
else
	echo "FAIL no tip without -check-liveness"; echo "${out}" | tail -5; status=1
fi

out=$("${GenMC}" -disable-estimation -check-liveness -- "${TEST}" 2>&1)
if echo "${out}" | grep -q 'Liveness violation' && ! echo "${out}" | grep -q "${TIP}"; then
	echo "ok   violation reported with -check-liveness"
else
	echo "FAIL expected a liveness violation and no tip"; echo "${out}" | tail -5; status=1
fi
exit ${status}
