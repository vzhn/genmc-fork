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
# Dependencies. Under a dependency-tracking model (-f -imm, -f -lkmm), GenMC
# also records, for each pass, a "<pass>+deps-add" and a "<pass>+deps-drop"
# pair, in which every event is preceded by a call with the masks of the
# sources (reads, allocations, markers) its address, data and control
# dependencies are on, as the interpreter computes them (passes/PassChecker.cpp).
# The add pair fails iff the pass gave some event a dependency it did not
# have, the drop pair iff it took one away; the two are joined here into one
# verdict per pass run:
#   added     the pass adds dependencies: GenMC then orders more than the
#             model (herd7 reads the source) and loses executions (#78);
#   dropped   it drops some: GenMC orders less, explores more, and may
#             report an execution the model forbids;
#   changed   both (a dependency changed kind: a branch turned into a select
#             converts control into data; or an event moved between
#             branches);
#   kept      both pairs proved;
#   unproved  a pair timed out or hit Alive2's limits (nothing shown wrong);
#   error     alive-tv could not read a pair.
# Known positive: fuzz seed 506 of scripts/lkmm/fuzz-ctrl.py under -lkmm
# (both register modes of scripts/lkmm/litmus2c.py) reports JumpThreadingPass
# as adding a control dependency to the write of z of P1 (issue #78).
#
# Triage of the dependency checks (this branch; one line per pass):
#   TRIAGE-TABLE
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
# The dependency pairs: their proofs are longer (a mask per event), and
# GenMC's inputs are never undef
ALIVE_DEPS_FLAGS="--src-unroll=2 --tgt-unroll=2 --smt-to=60000 --disable-undef-input"
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
	local n="$1" pass="$2" fun="$3" src="$4" tgt="$5" f="" flags="$ALIVE_FLAGS" r verdict
	[ "$fun" != "-" ] && f="--func=$fun"
	case "$pass" in *+deps-add | *+deps-drop) flags="$ALIVE_DEPS_FLAGS" ;; esac
	r=$(timeout 300 alive-tv $f $flags "$src" "$tgt" 2>&1)
	if [ $? = 124 ]; then
		verdict=timeout
	elif ! grep -q '^Summary:' <<<"$r"; then
		verdict=error # alive-tv did not finish (an unparsable module...)
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
		"$(grep -m1 -E '^(ERROR|LLVM ERROR)' <<<"$r" | cut -c1-100)"
}
export -f check_one
export ALIVE_FLAGS ALIVE_DEPS_FLAGS
cat "$OUT"/*/passes.txt 2>/dev/null |
	xargs -P "$JOBS" -L1 bash -c 'check_one "$@"' _ | sort >"$OUT/alive.tsv"

# 3. Summary
echo
echo "Pairs per pass and verdict (passes marked * change meaning on purpose):"
awk -F'\t' -v intended="$INTENDED" '
	BEGIN { split(intended, a, " "); for (i in a) mark[a[i]] = 1 }
	$2 ~ /\+deps-(add|drop)$/ { next }
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
	$2 ~ /\+deps-(add|drop)$/ { next }
	BEGIN { split(intended, a, " "); for (i in a) mark[a[i]] = 1 }
	$1 == "incorrect" && !($2 in mark) { print "  " $2 " " $3 ": " $4; print "    " $5 }' "$OUT/alive.tsv"
echo
echo "Failed claims of SpinAssumePass (the target states what its markers claim; it also"
echo "moves a CAS retry loop's initial load into the loop, which is reported here too):"
awk -F'\t' '$1 == "incorrect" && $2 == "SpinAssumePass" { print "  " $3 ": " $4; print "    " $5 }' "$OUT/alive.tsv"
echo
echo "Stale dominator trees: $(wc -l <"$OUT/domtree.txt")"

# 4. Dependencies: one verdict per pass run from its add and drop pairs (the
#    drop pair is recorded right after the add pair)
awk -F'\t' '
	$2 ~ /\+deps-(add|drop)$/ {
		n = split($4, p, "/"); file = p[n]; dir = substr($4, 1, length($4) - length(file))
		num = substr(file, 1, 4) + 0
		base = $2; sub(/\+deps-(add|drop)$/, "", base)
		if ($2 ~ /add$/) { k = dir num; add[k] = $1; pass[k] = base; fun[k] = $3; src[k] = $4 }
		else { k = dir (num - 1); drop[k] = $1 }
	}
	END {
		for (k in pass) {
			a = add[k]; d = drop[k]
			if (a == "error" || d == "error") c = "error"
			else if (a == "incorrect" && d == "incorrect") c = "changed"
			else if (a == "incorrect") c = "added"
			else if (d == "incorrect") c = "dropped"
			else if (a == "correct" && d == "correct") c = "kept"
			else c = "unproved"
			printf "%s\t%s\t%s\t%s\n", c, pass[k], fun[k], src[k]
		}
	}' "$OUT/alive.tsv" | sort >"$OUT/deps.tsv"
if [ -s "$OUT/deps.tsv" ]; then
	echo
	echo "Dependency changes per pass (the +deps pairs; see the header):"
	awk -F'\t' '
		BEGIN { split("kept added dropped changed unproved error", vs, " ") }
		{ n[$2][$1]++; passes[$2] = 1 }
		END {
			printf "%-32s", "pass"; for (i in vs) printf " %9s", vs[i]; print ""
			for (p in passes) {
				printf "%-32s", p
				for (i in vs) printf " %9d", n[p][vs[i]]
				print ""
			}
		}' "$OUT/deps.tsv"
	echo
	echo "Passes that add dependencies (GenMC orders more than the model: lost executions):"
	awk -F'\t' '$1 == "added" { print "  " $2 " " $3 ": " $4 }' "$OUT/deps.tsv"
	echo
	echo "Passes that drop or change dependencies (GenMC orders less: more executions, possible"
	echo "false alarms; a changed one may be a control dependency turned into a data one):"
	awk -F'\t' '$1 == "dropped" || $1 == "changed" { print "  " $1 " " $2 " " $3 ": " $4 }' "$OUT/deps.tsv"
fi
echo
echo "Results in $OUT"
