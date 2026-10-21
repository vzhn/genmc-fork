#!/bin/bash
#
# Proves executeRMWBinOp() (genmc/Support/RMWOps.hpp) equal to LLVM's
# atomicrmw for every input, with Alive2.
#
# For each operation and access size, a harness calls executeRMWBinOp() on
# operands zero-extended to the access width (as GenMC keeps values), and a
# reference computes what atomicrmw stores, in plain fixed-width C++. Both are
# compiled with clang to LLVM IR, where the harness inlines to a few
# instructions, and alive-tv proves each harness equivalent to its reference
# for all 64-bit inputs.
#
# Usage: scripts/prove-rmw-ops.sh [-c clang++] [-i config-include-dir]
#   clang++: a clang of the LLVM major version alive-tv was built against
#            (default: clang++ in PATH)
#   config-include-dir: a build's include directory, for genmc/config.h
#            (default: build/include)
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.

set -eu

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(dirname "$DIR")"
CLANG=clang++
CONFIG_INC="${ROOT}/build/include"

while getopts "c:i:" opt; do
	case $opt in
	c) CLANG="$OPTARG" ;;
	i) CONFIG_INC="$OPTARG" ;;
	*) exit 2 ;;
	esac
done
command -v alive-tv >/dev/null || { echo "alive-tv not found in PATH" >&2; exit 2; }
[ -f "$CONFIG_INC/genmc/config.h" ] || { echo "no genmc/config.h in $CONFIG_INC (pass -i)" >&2; exit 2; }

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

OPS="Xchg Add Sub And Nand Or Xor Max Min UMax UMin"

# The reference: what atomicrmw stores, at N bits, zero-extended
{
	echo '#include <cstdint>'
	for bytes in 1 2 4 8; do
		bits=$((bytes * 8))
		u="uint${bits}_t"
		s="int${bits}_t"
		for op in $OPS; do
			case $op in
			Xchg) e="y" ;;
			Add) e="$u(x + y)" ;;
			Sub) e="$u(x - y)" ;;
			And) e="$u(x & y)" ;;
			Nand) e="$u(~(x & y))" ;;
			Or) e="$u(x | y)" ;;
			Xor) e="$u(x ^ y)" ;;
			Max) e="$s(x) > $s(y) ? x : y" ;;
			Min) e="$s(x) < $s(y) ? x : y" ;;
			UMax) e="x > y ? x : y" ;;
			UMin) e="x < y ? x : y" ;;
			esac
			echo "extern \"C\" uint64_t rmw_${op}_${bits}(uint64_t a, uint64_t b)"
			echo "{ $u x = $u(a), y = $u(b); return uint64_t($u($e)); }"
		done
	done
} >"$WORK/ref.cpp"

# The harness: GenMC's executeRMWBinOp() on zero-extended operands
{
	echo '#include "genmc/Support/RMWOps.hpp"'
	echo '#include <cstdint>'
	for bytes in 1 2 4 8; do
		bits=$((bytes * 8))
		mask=$([ $bits = 64 ] && echo '~uint64_t(0)' || echo "((uint64_t(1) << $bits) - 1)")
		for op in $OPS; do
			echo "extern \"C\" uint64_t rmw_${op}_${bits}(uint64_t a, uint64_t b)"
			echo "{ return executeRMWBinOp(SVal(a & $mask), SVal(b & $mask), ASize($bytes), RMWBinOp::$op).get(); }"
		done
	done
} >"$WORK/genmc.cpp"

FLAGS="-std=gnu++23 -O2 -DNDEBUG -fno-exceptions -S -emit-llvm"
"$CLANG" $FLAGS "$WORK/ref.cpp" -o "$WORK/ref.ll"
"$CLANG" $FLAGS -I "$ROOT/genmc" -I "$CONFIG_INC" "$WORK/genmc.cpp" -o "$WORK/genmc.ll"

out=$(alive-tv "$WORK/ref.ll" "$WORK/genmc.ll" 2>&1)
echo "$out" | grep -E '^ +[0-9]+ (correct|incorrect|failed-to-prove|Alive2 errors)'
if echo "$out" | grep -qE '^ +[1-9][0-9]* (incorrect|failed-to-prove|Alive2 errors)'; then
	echo "$out" | awk '/^define/ { f = $0 } /^ERROR/ { print f; print "  " $0 }' | sort -u
	exit 1
fi
echo "executeRMWBinOp() equals atomicrmw for all inputs ($(echo $OPS | wc -w) operations x 4 sizes)"
