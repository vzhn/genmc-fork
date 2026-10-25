#!/bin/bash

# Runs every correct test under every model and exploration mode
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# Each test of tests/correct runs under -rc11, -sc, -tso, -ra, -imm and -lkmm, with the
# default exploration and with -exhaustive, under -check-invariants, for at
# most MATRIX_TIMEOUT seconds (default 5). A run fails the check only if it
# crashes, reports an internal failure or violates an invariant: the counts
# are checked elsewhere, a timeout says nothing, and a test may not compile
# or may be rejected under a model it was not written for.
#
# #223 crashed -lkmm (which explores as -exhaustive does) on a test that ran
# only under -imm: no test ran that combination of model and mode. -tso and
# -ra were left out until a non-atomic write outside coherence crashed the
# TSO checker under -check-invariants. ctest
# target "config-matrix" (label "matrix", not part of "fast"). The runs
# known to fail, each with its issue, are listed in config-matrix-known.txt;
# a new failure, or a listed one that passes, fails the check.
#
#   GenMC=<build>/bin/genmc scripts/check-config-matrix.sh [<test dir>...]

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
TIMEOUT="${MATRIX_TIMEOUT:-5}"
JOBS="${MATRIX_JOBS:-$(nproc)}"
ROOT=$(realpath "$DIR/../tests/correct")

one() {
	local t=$1 model=$2 mode=$3
	local dir line args cargs out rc
	dir=$(dirname "$(dirname "$t")")
	line=""
	[ -f "$dir/args.$model.mo.in" ] && line=$(head -n1 "$dir/args.$model.mo.in")
	args=$(echo "$line" | cut -f1 -d'|')
	cargs=$(echo "$line" | cut -s -f2 -d'|')
	# shellcheck disable=SC2086
	out=$(ulimit -v 4000000; timeout "$TIMEOUT" "$GenMC" -"$model" $mode $args \
		-check-invariants -disable-estimation -- $cargs "$t" 2>&1 < /dev/null)
	rc=$?
	if [[ $rc -ge 128 && $rc -ne 124 && $rc -ne 137 ]] ||
	   echo "$out" | grep -qE "INTERNAL FAILURE|^Invariant .* violated"; then
		echo "FAIL -$model${mode:+ $mode} ${t#"$ROOT"/}: status $rc"
		echo "$out" | grep -m1 -E "^Invariant|INTERNAL|check failed" | sed 's/^/     /'
	fi
}
export -f one
export GenMC TIMEOUT ROOT

if [[ $# -gt 0 ]]; then mapfile -t dirs < <(realpath "$@"); else dirs=("$ROOT"/*); fi
tests=$(find "${dirs[@]}" -path '*/variants/*.c' | sort)
n=$(echo "$tests" | wc -l)
out=$(for model in rc11 sc tso ra imm lkmm; do
	for mode in "" "-exhaustive"; do
		# no trailing blank: xargs -L 1 would join the line with the next
		echo "$tests" | sed "s|\$| $model${mode:+ $mode}|"
	done
done | xargs -P "$JOBS" -L 1 bash -c 'one "$0" "$1" "$2"')
KNOWN="${MATRIX_KNOWN:-$DIR/config-matrix-known.txt}"
known=$(grep -vE '^\s*(#|$)' "$KNOWN" 2>/dev/null | sed -E 's/ +#[0-9]+$//' | sort)
failing=$(echo "$out" | sed -nE 's/^FAIL (.*): status .*/\1/p' | sed 's/  */ /g' | sort)
new=$(comm -23 <(echo "$failing") <(echo "$known") | sed '/^$/d')
fixed=$(comm -13 <(echo "$failing") <(echo "$known") | sed '/^$/d')
status=0
if [[ -n "$new" ]]; then
	echo "$out"
	echo "config-matrix: new failures (not in $(basename "$KNOWN")):"
	echo "$new" | sed 's/^/  /'
	status=1
fi
if [[ -n "$fixed" ]]; then
	echo "config-matrix: these listed failures now pass; remove them from $(basename "$KNOWN"):"
	echo "$fixed" | sed 's/^/  /'
	status=1
fi
[[ $status -eq 0 ]] && echo "config-matrix: ok ($n tests x 12 configurations, $(echo "$known" | sed '/^$/d' | wc -l) known failures)"
exit $status
