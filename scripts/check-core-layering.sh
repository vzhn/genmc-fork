#!/bin/bash
#
# Checks the include layering of the core library (genmc/genmc):
#
#   layer 0: ADT, Support    -- containers, values, errors
#   layer 1: Execution       -- events, labels, graphs, memory models
#   layer 2: Verification    -- exploration, driver, configuration
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
	Verification) echo 2 ;;
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
