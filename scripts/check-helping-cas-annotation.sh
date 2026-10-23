#!/bin/bash

# Checks that GenMC rejects wrong helped/helping CAS annotations
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# A misannotated helping CAS is a user error (exit status 17, not a
# verification error, so runwrong.sh cannot run these): a helping CAS
# without a helped one, a store to the location unordered with the
# helping CAS, and a store before the helped CAS that the helping CAS
# does not see. A correct annotation must pass. ctest target
# "helping-cas-annotation".
#
#   GenMC=<build>/bin/genmc scripts/check-helping-cas-annotation.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
WRONG="${DIR}/../tests/wrong/saver"
ERR='Helped/Helping CAS annotation error!'

status=0
check() { # <program> <expected status> <text the output must contain>
	local prog="$1" want="$2" what="$3" model out rc
	for model in rc11 imm sc; do
		out=$("${GenMC}" "-${model}" -disable-estimation -- "${prog}" 2>&1)
		rc=$?
		if [[ "${rc}" -eq "${want}" ]] && echo "${out}" | grep -qF -- "${what}"; then
			echo "ok   $(basename "${prog}") -${model}"
		else
			echo "FAIL $(basename "${prog}") -${model}: status ${rc}, expected ${want} and \"${what}\""
			echo "${out}" | tail -3; status=1
		fi
	done
}

check "${WRONG}/help-annot/variants/help-annot0.c" 17 "${ERR} Does helped CAS always execute?"
check "${WRONG}/help-annot2/variants/help-annot20.c" 17 "${ERR} Unordered store to helping CAS location!"
check "${WRONG}/help-annot3/variants/help-annot30.c" 17 \
	"${ERR} Not all stores before helped-CAS are visible to helping-CAS!"
check "${DIR}/../tests/correct/saver/help-cas/variants/help-cas0.c" 0 "No errors were detected"
exit ${status}
