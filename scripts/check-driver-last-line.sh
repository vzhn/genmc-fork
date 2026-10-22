#!/bin/bash

# The test scripts read args.<model>.mo.in and expected.<model>.mo.in line by
# line; a last line without a trailing newline is a configuration too (issue
# #176: it was skipped, and with a single line the test never ran). Builds a
# category of three tests whose last lines have no newline and checks that
# driver.sh and oracle-diff.sh run every configuration. ctest target
# "driver-last-line".
#
#   GenMC=<build>/bin/genmc scripts/check-driver-last-line.sh
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"

tmp=$(mktemp -d)
trap 'rm -rf "${tmp}"' EXIT
mkdir -p "${tmp}/empty" "${tmp}/cat"

# mktest <name> <args.rc11.mo.in contents> <expected.rc11.mo.in contents>
mktest() {
	mkdir -p "${tmp}/cat/$1/variants"
	echo 'int main(void) { return 0; }' > "${tmp}/cat/$1/variants/${1}0.c"
	printf '%b' "$2" > "${tmp}/cat/$1/args.rc11.mo.in"
	printf '%b' "$3" > "${tmp}/cat/$1/expected.rc11.mo.in"
}
mktest two-args '  | -DN=1\n  | -DN=2' '1\n1\n'	# the last of two args lines
mktest one-arg  '  |'                    '1\n'	# the only args line
mktest expected '  |\n'                  '1'	# the only expected line

status=0
check() {
	if [ "$2" = "$3" ]; then
		echo "ok   $1: $2"
	else
		echo "FAIL $1: $2, expected $3"
		status=1
	fi
}

out=$(MODELS=rc11 CORRECT_DIRS="${tmp}/cat" WRONG_DIRS="${tmp}/empty" GenMC="${GenMC}" \
	"${DIR}/driver.sh" --debug 2>&1 | sed 's/\x1b\[[0-9;]*m//g')
# (a test with an args file is named <test>/<N> by its -DN=, else <test>/)
for row in two-args/1 two-args/2 one-arg/ expected/; do
	check "driver.sh runs ${row}" "$(echo "${out}" | grep -cE "^\| ${row} +\| SAFE")" 1
done
if ! echo "${out}" | grep -q "Testing proceeded as expected"; then
	echo "FAIL driver.sh:"
	echo "${out}"
	status=1
fi

runs=$("${DIR}/oracle-diff.sh" -m rc11 "${GenMC}" "${tmp}/cat" 2>&1 |
	awk '/^oracle-diff: / { print $2 }')
check "oracle-diff.sh runs" "${runs}" 4

exit ${status}
