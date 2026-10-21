#!/bin/bash
#
# Checks GenMC's transformation passes with Alive2 (translation validation).
#
# For each test program, runs GenMC with -output-llvm-passes (each pass that
# changes the module leaves the module before and after it) and
# -verify-pass-analyses (dominator trees a pass declares preserved must be
# current), then asks alive-tv whether each pass's result refines its input.
# Atomic accesses are calls in the dumped modules (Alive2 has no atomics), so
# a pass may not add, drop, merge or reorder them.
#
# Passes that change the program's meaning on purpose are reported apart:
# spin-assume and loop unrolling block executions, StrengthenCAS strengthens
# orderings, PropagateAssumes moves assumptions.
#
# Usage: scripts/check-passes.sh [-g genmc] [-o outdir] [-j jobs] [-t secs]
#            [-f "genmc flags"] file.c...
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.

set -u

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GENMC="${DIR}/../build/bin/genmc"
OUT="$(mktemp -d)"
JOBS=$(nproc)
TIMEOUT=30
FLAGS="-rc11"
ALIVE_FLAGS="--src-unroll=2 --tgt-unroll=2 --smt-to=20000"
INTENDED="SpinAssumePass LoopUnrollPass StrengthenCASPass PropagateAssumesPass"

while getopts "g:o:j:t:f:" opt; do
	case $opt in
	g) GENMC="$OPTARG" ;;
	o) OUT="$OPTARG" ;;
	j) JOBS="$OPTARG" ;;
	t) TIMEOUT="$OPTARG" ;;
	f) FLAGS="$OPTARG" ;;
	*) exit 2 ;;
	esac
done
shift $((OPTIND - 1))
[ $# -gt 0 ] || { echo "usage: $0 [-g genmc] [-o outdir] [-j jobs] [-t secs] [-f flags] file.c..." >&2; exit 2; }
command -v alive-tv >/dev/null || { echo "alive-tv not found in PATH" >&2; exit 2; }
mkdir -p "$OUT"

# 1. Dump the passes of each test (exploration is cut by the timeout: the
#    dumps are written during transformation)
dump_one() {
	local test="$1" name
	name=$(echo "$test" | sed 's|^\./||; s|/|_|g; s|\.c$||')
	timeout "$TIMEOUT" "$GENMC" $FLAGS -verify-pass-analyses \
		-output-llvm-passes="$OUT/$name" -- "$test" >"$OUT/$name.genmc.log" 2>&1
	if grep -q 'stale dominator tree' "$OUT/$name.genmc.log"; then
		echo "STALE-DOMTREE $test: $(grep -m1 'stale dominator tree' "$OUT/$name.genmc.log")"
	fi
}
export -f dump_one
export GENMC OUT TIMEOUT FLAGS
printf '%s\n' "$@" | xargs -P "$JOBS" -I{} bash -c 'dump_one "$@"' _ {} | tee "$OUT/domtree.txt"

# 2. Validate every recorded pair
check_one() {
	local n="$1" pass="$2" fun="$3" src="$4" tgt="$5" f="" r verdict
	[ "$fun" != "-" ] && f="--func=$fun"
	r=$(timeout 300 alive-tv $f $ALIVE_FLAGS "$src" "$tgt" 2>&1)
	if [ $? = 124 ]; then
		verdict=timeout
	elif grep -qE '^ +[1-9][0-9]* incorrect' <<<"$r"; then
		verdict=incorrect
	elif grep -qE '^ +[1-9][0-9]* failed-to-prove' <<<"$r"; then
		verdict=unproved
	elif grep -qE '^ +[1-9][0-9]* Alive2 errors' <<<"$r"; then
		verdict=error
	else
		verdict=correct
	fi
	printf '%s\t%s\t%s\t%s\t%s\n' "$verdict" "$pass" "$fun" "$src" \
		"$(grep -m1 -E '^ERROR' <<<"$r" | cut -c1-100)"
}
export -f check_one
export ALIVE_FLAGS
cat "$OUT"/*/passes.txt 2>/dev/null |
	xargs -P "$JOBS" -L1 bash -c 'check_one "$@"' _ | sort >"$OUT/alive.tsv"

# 3. Summary
echo
echo "Pairs per pass and verdict (passes marked * change meaning on purpose):"
awk -F'\t' -v intended="$INTENDED" '
	BEGIN { split(intended, a, " "); for (i in a) mark[a[i]] = 1 }
	{ n[$2][$1]++; passes[$2] = 1; verdicts[$1] = 1 }
	END {
		printf "%-32s", "pass"; for (v in verdicts) printf " %10s", v; print ""
		for (p in passes) {
			printf "%-32s", p (p in mark ? " *" : "")
			for (v in verdicts) printf " %10d", n[p][v]
			print ""
		}
	}' "$OUT/alive.tsv"
echo
echo "Incorrect results of passes that should preserve meaning:"
awk -F'\t' -v intended="$INTENDED" '
	BEGIN { split(intended, a, " "); for (i in a) mark[a[i]] = 1 }
	$1 == "incorrect" && !($2 in mark) { print "  " $2 " " $3 ": " $4; print "    " $5 }' "$OUT/alive.tsv"
echo
echo "Failed claims of SpinAssumePass (the target states what its markers claim; it also"
echo "moves a CAS retry loop's initial load into the loop, which is reported here too):"
awk -F'\t' '$1 == "incorrect" && $2 == "SpinAssumePass" { print "  " $3 ": " $4; print "    " $5 }' "$OUT/alive.tsv"
echo
echo "Stale dominator trees: $(wc -l <"$OUT/domtree.txt")"
echo "Results in $OUT"
