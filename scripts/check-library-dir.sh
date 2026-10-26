#!/bin/bash

# Checks that -library-dir reports the events of code in a header there at
# the line of the program that called it
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# A tool that reads the error report needs the program's lines: the SV-COMP
# runner (scripts/svcomp) adds functions in headers of its own, and a
# witness may only name lines of the task. The race of
# tests/wrong/infr/library-dir is in a header's function; without the
# option it is reported at the header's line. ctest target "library-dir".
#
#   GenMC=<build>/bin/genmc scripts/check-library-dir.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
VARIANTS="${DIR}/../tests/wrong/infr/library-dir/variants"

run() {
	"${GenMC}" -disable-estimation -disable-mm-detector "$@" -- "${VARIANTS}/library-dir.c" 2>&1
}

status=0
check() {
	local what=$1 out=$2
	shift 2
	for want in "$@"; do
		if echo "${out}" | grep -qF "${want}"; then
			echo "ok   ${what}: ${want}"
		else
			echo "FAIL ${what}: expected \"${want}\""
			status=1
		fi
	done
}
check "-library-dir" "$(run -library-dir="${VARIANTS}/lib")" \
	"(0, 2): Wna (x, 2) library-dir.c:25" "(1, 1): Wna (x, 1) library-dir.c:16"
check "without it" "$(run)" "(0, 2): Wna (x, 2) store.h:7" "(1, 1): Wna (x, 1) store.h:7"
exit ${status}
