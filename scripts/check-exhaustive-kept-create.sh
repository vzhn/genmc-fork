#!/bin/bash

# Checks that -exhaustive under -imm takes no inconsistent revisit for a
# thread that has only its begin
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# A begin keeps nothing: a revisit that kept the create of a thread that had
# not run (without the join before it) made graphs that are not consistent.
# The execution count does not show it, the exhaustive statistics do.
# ctest target "exhaustive-kept-create".
#
#   GenMC=<build>/bin/genmc scripts/check-exhaustive-kept-create.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
TEST="${DIR}/../tests/correct/infr/imm-exhaustive-kept-create/variants/imm-exhaustive-kept-create.c"

out=$("${GenMC}" -imm -exhaustive -disable-estimation -- "${TEST}" 2>&1)
stats=$(echo "${out}" | grep '^Exhaustive:')
if [ -n "${stats}" ] && ! echo "${stats}" | grep -q 'inconsistent'; then
	echo "ok   no inconsistent revisit"
	exit 0
fi
echo "FAIL expected no inconsistent revisit"; echo "${out}" | tail -5
exit 1
