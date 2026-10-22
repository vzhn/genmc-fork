#!/bin/bash
#
# Checks the include layering of the core library (genmc/genmc):
#
#   layer 0: ADT, Support    -- containers, values, errors   (genmc_support)
#   layer 1: Execution       -- events, labels, graphs        (genmc_graph)
#   layer 2: Models          -- the memory models             (genmc_models)
#   layer 3: Verification    -- exploration, driver, configuration (genmc_verification)
#
# A core file may include headers of its own layer or below, and never an
# LLVM, interpreter (Runtime/, lli) or pass (passes/) header. Upward includes
# that exist today are listed in core-layering-allow.txt; the check fails on
# any new one and on allowlist entries that are no longer needed, so the
# list can only shrink.
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
ROOT="${DIR}/../genmc"
ALLOW="${DIR}/core-layering-allow.txt"

layer() {
	case "$1" in
	ADT | Support) echo 0 ;;
	Execution) echo 1 ;;
	Models) echo 2 ;;
	Verification) echo 3 ;;
	*) echo 9 ;;
	esac
}

status=0
found=$(mktemp)
trap 'rm -f "${found}"' EXIT

cd "${ROOT}" || exit 1
while IFS= read -r file; do
	dir=${file#genmc/}
	dir=${dir%%/*}
	own=$(layer "${dir}")
	while IFS= read -r inc; do
		case "${inc}" in
		llvm/* | Runtime/* | passes/* | lli*)
			echo "FAIL: ${file} includes ${inc} (the core library must not depend on LLVM, the interpreter or the passes)"
			status=1
			continue
			;;
		genmc/*) ;;
		*) continue ;;
		esac
		target=${inc#genmc/}
		target=${target%%/*}
		[[ "${target}" == *.h || "${target}" == *.hpp ]] && continue # config headers
		if (($(layer "${target}") > own)); then
			echo "${file} -> ${inc}" >> "${found}"
		fi
	done < <(sed -nE 's/^[[:space:]]*#[[:space:]]*include[[:space:]]*[<"]([^>"]+)[>"].*/\1/p' "${file}")
done < <(find genmc -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.def' | sort)

# No declaration or use of LLVM's namespace either (a forward declaration
# lets an LLVM type into a core interface without an include)
while IFS= read -r hit; do
	echo "FAIL: ${hit} (the core library must not name LLVM types)"
	status=1
done < <(grep -rnE '^[^/*]*(namespace[[:space:]]+llvm|llvm::)' genmc)

# Each layer is one directory (two for layer 0) and one CMake target: every
# source file must be in the source list of its directory's target
# (genmc/CMakeLists.txt), and every listed file must be in that directory
target_of() {
	case "$1" in
	ADT | Support) echo SUPPORT ;;
	Execution) echo GRAPH ;;
	Models) echo MODELS ;;
	Verification) echo VERIFICATION ;;
	*) echo NONE ;;
	esac
}
lists="${DIR}/../genmc/CMakeLists.txt"
while IFS= read -r src; do
	dir=${src#genmc/}
	dir=${dir%%/*}
	want=$(target_of "${dir}")
	got=$(awk -v f="${src}" '/^set\(GENMC_[A-Z]+_SOURCES/{l=$1; sub(/set\(GENMC_/,"",l); sub(/_SOURCES/,"",l)} /^\)/{l=""} l!="" && $1==f{print l}' "${lists}")
	[[ "${got}" == "${want}" ]] ||
		{ echo "FAIL: ${src} is in the source list of ${got:-no target}, its directory is ${want}"; status=1; }
done < <(find genmc -name '*.cpp' | sort)
while IFS= read -r src; do
	[[ -f "${src}" ]] || { echo "FAIL: ${src} is in genmc/CMakeLists.txt but does not exist"; status=1; }
done < <(awk '/^set\(GENMC_[A-Z]+_SOURCES/{l=1; next} /^\)/{l=0} l{print $1}' "${lists}")

sort -o "${found}" "${found}"
while IFS= read -r line; do
	grep -qxF "${line}" <(grep -v '^#' "${ALLOW}") ||
		{ echo "FAIL: new upward include: ${line}"; status=1; }
done < "${found}"
while IFS= read -r line; do
	grep -qxF "${line}" "${found}" ||
		{ echo "FAIL: stale allowlist entry (remove it from $(basename "${ALLOW}")): ${line}"; status=1; }
done < <(grep -v '^#' "${ALLOW}" | grep -v '^$')

[[ ${status} -eq 0 ]] && echo "core layering: OK ($(wc -l < "${found}") allowlisted upward includes)"
exit ${status}
