#!/bin/bash
#
# Regenerates GenMC's consistency checkers with Kater from its kat files
# (<kater>/kat/<model>-genmc.kat), formatted with the repository's
# clang-format style. The kat files stay in Kater: they are GPL-3.0, and so
# is a model transcribed from the kernel's linux-kernel.cat (GPL-2.0+).
#
# Usage: kater-export.sh [--install | -o <out-dir>] [MODEL...]
#
# MODEL is a kat file's base name without "-genmc.kat" (default: sc tso ra
# rc11 imm); the checker class is its upper-case name (SCChecker, ...).
# The files go to <out-dir> (default: a new temporary directory); with
# --install they replace the checked-in ones. Environment: KATER_SRC, a
# built Kater checkout (default: ~/.local/src/kater; binary in Release/).
#
# GenMC 0.19's checker interface needs Kater's genmc-0.19-export branch
# (see README.md); the public Kater 0.4.0 exports for an older interface.

set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${DIR}/../.." && pwd)"
CONS="${ROOT}/genmc/genmc/Execution/Consistency"
KATER_SRC="${KATER_SRC:-${HOME}/.local/src/kater}"
KATER="${KATER_SRC}/Release/kater"

out=""
if test "${1:-}" = "--install"; then
	out="${CONS}"
	shift
elif test "${1:-}" = "-o"; then
	out="$2"
	shift 2
fi
test -n "${out}" || out="$(mktemp -d)"
mkdir -p "${out}"
models=("$@")
test ${#models[@]} -gt 0 || models=(sc tso ra rc11 imm)

tmp="$(mktemp -d)"
trap 'rm -rf "${tmp}"' EXIT
for m in "${models[@]}"; do
	name="${m^^}"
	"${KATER}" -e -n "${name}" -p "${tmp}" "${KATER_SRC}/kat/${m}-genmc.kat" > "${tmp}/${m}.log" 2>&1 ||
		{ cat "${tmp}/${m}.log" >&2; exit 1; }
	for f in "${name}Checker.hpp" "${name}Checker.cpp"; do
		clang-format --style="file:${ROOT}/.clang-format" --assume-filename="${f}" \
			"${tmp}/${f}" > "${out}/${f}"
	done
	echo "${name}: ${out}/${name}Checker.{hpp,cpp}"
done
