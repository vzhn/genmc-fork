#!/bin/bash

# Checks that the transformation passes keep the dependencies of LKMM programs
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# Runs scripts/check-passes.sh under -lkmm on a few programs whose passes are
# known to keep every dependency (see the triage table in its header), and
# fails if a pass is reported as adding, dropping or changing one, or if a
# pair could not be checked. Needs alive-tv (Alive2) in PATH: without it the
# test is skipped (exit 77). ctest target "check-pass-deps".
#
#   GenMC=<build>/bin/genmc scripts/check-pass-deps.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
TESTS="${DIR}/../tests/correct/lkmm"
PROGRAMS="ctrl-lb-call ctrl-lb-loop ctrl-early-return lb-dep-xchg CoRR+poonceonce+Once \
	atomic-00 cas-fail-then-write create-after-read"

command -v alive-tv >/dev/null || { echo "SKIP alive-tv not found in PATH"; exit 77; }

out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
files=()
for p in $PROGRAMS; do
	files+=("${TESTS}/${p}/variants/${p}.c")
done
"${DIR}/check-passes.sh" -g "${GenMC}" -o "${out}" -j 4 -t 30 -f "-lkmm -disable-estimation" \
	"${files[@]}" >"${out}/report.txt" 2>&1 || { cat "${out}/report.txt"; exit 1; }

status=0
if [ ! -s "${out}/deps.tsv" ]; then
	echo "FAIL no dependency pairs were recorded"; cat "${out}/report.txt"; exit 1
fi
while IFS=$'\t' read -r verdict pass fun src; do
	case $verdict in
	kept | unproved) ;;
	*) echo "FAIL ${verdict}: ${pass} ${fun} (${src})"; status=1 ;;
	esac
done <"${out}/deps.tsv"
echo "ok   $(wc -l <"${out}/deps.tsv") pass runs checked: $(cut -f1 "${out}/deps.tsv" | sort | uniq -c | tr -s ' \n' ' ')"
[ ${status} = 0 ] || sed -n '/Dependency changes per pass/,$p' "${out}/report.txt"
exit ${status}
