#!/bin/bash

# Checks that GenMC warns about __VERIFIER_nondet_X() once per run
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# Each call to __VERIFIER_nondet_X() returns the same value in every
# execution, so a clean result does not cover the other values (#48).
# GenMC must say so once per run: with several calls, threads and
# executions, after the estimation and with several workers, and for the
# variants genmc.h does not declare. A program without such calls must get
# no warning. ctest target "nondet-warning".
#
#   GenMC=<build>/bin/genmc scripts/check-nondet-warning.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
TESTS="${DIR}/../tests/correct/infr"
WARN='GenMC explores one value per call'

status=0
check_once() { # <name> <program> <warning text it must name> <genmc flags...>
	local name="$1" prog="$2" what="$3"
	shift 3
	local out n
	out=$("${GenMC}" "$@" -- "${prog}" 2>&1)
	n=$(echo "${out}" | grep -c "${WARN}")
	if [[ "${n}" -eq 1 ]] && echo "${out}" | grep "${WARN}" | grep -qF "${what}"; then
		echo "ok   ${name}: warned once"
	else
		echo "FAIL ${name}: ${n} warnings, expected one naming ${what}"
		echo "${out}" | tail -5; status=1
	fi
}

VARIANTS="${TESTS}/nondet-variants/variants"
check_once "estimation and verification" "${VARIANTS}/nondet-variants0.c" \
	'__VERIFIER_nondet_int()'
check_once "two workers" "${VARIANTS}/nondet-variants0.c" '__VERIFIER_nondet_bool()' \
	-disable-estimation -nthreads=2
check_once "own declarations" "${VARIANTS}/nondet-variants1.c" '__VERIFIER_nondet_u32()' \
	-disable-estimation

out=$("${GenMC}" -- "${TESTS}/calloc-zeroed/variants/calloc-zeroed.c" 2>&1)
if echo "${out}" | grep -q "${WARN}"; then
	echo "FAIL no nondet calls: warned"; echo "${out}" | tail -5; status=1
else
	echo "ok   no nondet calls: no warning"
fi
exit ${status}
