#!/bin/bash

# Checks that -print-addresses prints the address of each access and the
# block of each allocation and free in the error graph, and that the graph
# is unchanged without it
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# A heap access is printed with an empty name, or one guessed from the
# allocation site, so a tool reading the graph could not tell which block an
# access or a free touches. ctest target "print-addresses".
#
#   GenMC=<build>/bin/genmc scripts/check-print-addresses.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
TEST="${DIR}/../tests/wrong/memory/access-freed-conflict/variants/access-freed-conflict.c"

status=0
# (without the wall-clock time, which differs between runs)
run() {
	"${GenMC}" -disable-estimation -disable-mm-detector "$@" -- "${TEST}" 2>&1 |
		grep -v "^Total wall-clock time"
}

with=$(run -print-addresses)
without=$(run)
for want in \
	"(0, 1): MALLOC  @H#(0, 16) size=4 " \
	"(0, 2): Wna (p @G#(0, 0), 16) " \
	"(0, 6): Wna (malloc@access-freed-conflict.c:19 @H#(0, 16), 1) " \
	"(1, 2): FREE @H#(0, 16) size=4 "; do
	if echo "${with}" | grep -qF "${want}"; then
		echo "ok   -print-addresses: ${want}"
	else
		echo "FAIL -print-addresses: expected \"${want}\""
		status=1
	fi
done
if echo "${without}" | grep -qE " @[GSH]I?#"; then
	echo "FAIL without -print-addresses, the graph has an address:"
	echo "${without}" | grep -E " @[GSH]I?#"
	status=1
elif [ "$(echo "${with}" | sed 's/ @[GSH]I\?#([0-9]*, [0-9]*)\( size=[0-9]*\)\?//')" != "${without}" ]; then
	echo "FAIL -print-addresses changes more than the addresses"
	diff <(echo "${without}") <(echo "${with}")
	status=1
else
	echo "ok   without -print-addresses, the graph is unchanged"
fi
exit ${status}
