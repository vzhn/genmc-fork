#!/bin/bash
#
# Proves the expression evaluator's arithmetic (genmc/Support/SExprOps.hpp)
# equal to LLVM's integer semantics for every input, with Alive2.
#
# For each operation and width, a harness calls the SExprOps function on
# operands zero-extended to the width (as GenMC keeps values), and a
# reference computes the LLVM result in plain fixed-width C++, with 0 where
# LLVM gives no value (division by zero, signed division overflow, a shift by
# the width or more), as the evaluator documents. Both are compiled with
# clang to LLVM IR, where the harness inlines to a few instructions, and
# alive-tv proves each harness equivalent to its reference for all 64-bit
# inputs.
#
# Usage: scripts/prove-sexpr-ops.sh [-c clang++] [-i config-include-dir]
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

# Operations with two operands, of the operands' width; and with one
BINOPS="add sub mul udiv sdiv urem srem band bor bxor shl lshr ashr eq ne ult ule ugt uge slt sle sgt sge"
UNOPS="lnot sext trunc"

# The reference: LLVM's result at N bits, zero-extended; x, y are the
# operands as unsigned, sx, sy as signed
{
	echo '#include <cstdint>'
	for bits in 8 16 32 64; do
		u="uint${bits}_t"
		s="int${bits}_t"
		for op in $BINOPS; do
			# Signed division at 32 bits is beyond the solver (the harness
			# divides at 64 bits and truncates): proved at 8, 16 and 64,
			# and checked at 32 on boundary values by the unit tests
			case ${op}_${bits} in sdiv_32 | srem_32) continue ;; esac
			case $op in
			add) e="$u(x + y)" ;;
			sub) e="$u(x - y)" ;;
			mul) e="$u(x * y)" ;;
			udiv) e="y == 0 ? 0 : x / y" ;;
			sdiv) e="(sy == 0 || (sy == -1 && sx == INT${bits}_MIN)) ? 0 : $u(sx / sy)" ;;
			urem) e="y == 0 ? 0 : x % y" ;;
			srem) e="(sy == 0 || sy == -1) ? 0 : $u(sx % sy)" ;;
			band) e="x & y" ;;
			bor) e="x | y" ;;
			bxor) e="x ^ y" ;;
			shl) e="y >= $bits ? 0 : $u(x << y)" ;;
			lshr) e="y >= $bits ? 0 : $u(x >> y)" ;;
			ashr) e="y >= $bits ? 0 : $u(sx >> y)" ;;
			eq) e="x == y" ;;
			ne) e="x != y" ;;
			ult) e="x < y" ;;
			ule) e="x <= y" ;;
			ugt) e="x > y" ;;
			uge) e="x >= y" ;;
			slt) e="sx < sy" ;;
			sle) e="sx <= sy" ;;
			sgt) e="sx > sy" ;;
			sge) e="sx >= sy" ;;
			esac
			echo "extern \"C\" uint64_t op_${op}_${bits}(uint64_t a, uint64_t b)"
			echo "{ $u x = $u(a), y = $u(b); $s sx = $s(x), sy = $s(y); (void)sx; (void)sy; return uint64_t($u($e)); }"
		done
		for op in $UNOPS; do
			case $op in
			lnot) e="$u(x == 0)" ;;
			sext) e="uint64_t(int64_t(sx))" ;;
			trunc) e="x" ;;
			esac
			echo "extern \"C\" uint64_t op_${op}_${bits}(uint64_t a)"
			echo "{ $u x = $u(a); $s sx = $s(x); (void)sx; return uint64_t($e); }"
		done
	done
} >"$WORK/ref.cpp"

# The harness: SExprOps on zero-extended operands of width N
{
	echo '#include "genmc/Support/SExprOps.hpp"'
	echo '#include <cstdint>'
	for bits in 8 16 32 64; do
		mask=$([ $bits = 64 ] && echo '~uint64_t(0)' || echo "((uint64_t(1) << $bits) - 1)")
		for op in $BINOPS; do
			case ${op}_${bits} in sdiv_32 | srem_32) continue ;; esac
			case $op in
			udiv | urem | band | bor | bxor | eq | ne | ult | ule | ugt | uge) args="" ;;
			*) args=", $bits" ;;
			esac
			echo "extern \"C\" uint64_t op_${op}_${bits}(uint64_t a, uint64_t b)"
			echo "{ return SExprOps::${op}(SVal(a & $mask), SVal(b & $mask)${args}).get(); }"
		done
		echo "extern \"C\" uint64_t op_lnot_${bits}(uint64_t a) { return SExprOps::lnot(SVal(a & $mask)).get(); }"
		echo "extern \"C\" uint64_t op_sext_${bits}(uint64_t a) { return SExprOps::sextTo(SVal(a & $mask), $bits, 64).get(); }"
		echo "extern \"C\" uint64_t op_trunc_${bits}(uint64_t a) { return SExprOps::trunc(SVal(a), $bits).get(); }"
	done
} >"$WORK/genmc.cpp"

FLAGS="-std=gnu++23 -O2 -DNDEBUG -fno-exceptions -S -emit-llvm"
"$CLANG" $FLAGS "$WORK/ref.cpp" -o "$WORK/ref.ll"
"$CLANG" $FLAGS -I "$ROOT/genmc" -I "$CONFIG_INC" "$WORK/genmc.cpp" -o "$WORK/genmc.ll"

# Division needs more than the default solver budget
out=$(alive-tv --smt-to=60000 "$WORK/ref.ll" "$WORK/genmc.ll" 2>&1)
echo "$out" | grep -E '^ +[0-9]+ (correct|incorrect|failed-to-prove|Alive2 errors)'
if echo "$out" | grep -qE '^ +[1-9][0-9]* (incorrect|failed-to-prove|Alive2 errors)'; then
	echo "$out" | awk '/^define/ { f = $0 } /^ERROR/ { print f; print "  " $0 }' | sort -u
	exit 1
fi
echo "SExprOps equals LLVM's semantics for all inputs ($(echo $BINOPS $UNOPS | wc -w) operations x 4 widths, signed division at 8, 16 and 64 bits)"
