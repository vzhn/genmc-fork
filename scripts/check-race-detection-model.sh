#!/bin/bash

# Checks that -disable-race-detection keeps the memory model asked for
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# The memory-model detector moves a program to a stronger model when the
# orderings it uses allow it (SC for a program without atomics), which
# keeps the outcomes of programs without races only. -disable-race-detection
# explores racy programs, so the detector must keep the model: under -rc11
# and -imm the racy plain message passing of plain-race-ignored violates
# its assertion, under SC it does not. With race detection, the detector
# still adjusts the model (and the race is reported). ctest target
# "race-detection-model".
#
#   GenMC=<build>/bin/genmc scripts/check-race-detection-model.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
TEST="${DIR}/../tests/wrong/safety/plain-race-ignored/variants/plain-mp.c"
TIP='Automatically adjusting memory model'

status=0
for m in rc11 imm; do
	out=$("${GenMC}" -${m} -disable-race-detection -disable-estimation -- "${TEST}" 2>&1)
	if echo "${out}" | grep -q "${TIP}"; then
		echo "FAIL -${m}: adjusted the model without race detection"; echo "${out}" | tail -5; status=1
	elif ! echo "${out}" | grep -q 'Safety violation'; then
		echo "FAIL -${m}: no safety violation"; echo "${out}" | tail -5; status=1
	else
		echo "ok   -${m}: model kept, safety violation found"
	fi
done

out=$("${GenMC}" -rc11 -disable-estimation -- "${TEST}" 2>&1)
if echo "${out}" | grep -q "${TIP}" && echo "${out}" | grep -q 'Non-atomic race'; then
	echo "ok   -rc11 with race detection: model adjusted, race reported"
else
	echo "FAIL -rc11 with race detection: expected the model adjusted and the race reported"
	echo "${out}" | tail -5; status=1
fi
exit ${status}
