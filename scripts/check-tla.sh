#!/bin/bash
#
# Checks the TLA+ models in models/tla with TLC.
#
# Each configuration (<Module>-<name>.cfg) carries an "\* EXPECT:" line: "pass"
# means TLC must find no error, any other word names the invariant TLC must
# report as violated (a past bug, brought back by the configuration's
# switches). The script fails if any configuration deviates from its line,
# so the models stay tied to the bugs they were written for.
#
# Usage: scripts/check-tla.sh [-j workers] [models/tla/Name.cfg...]
# Environment: TLC (default: tlc in PATH, the wrapper around tla2tools.jar),
#              TLA_JAVA_OPTS (passed on by the wrapper).
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

set -u

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODELS="$(cd "${DIR}/../models/tla" && pwd)"
TLC="${TLC:-tlc}"
jobs=2

while getopts "j:" opt; do
	case "${opt}" in
	j) jobs="${OPTARG}" ;;
	*) echo "usage: $0 [-j workers] [config.cfg...]" >&2; exit 2 ;;
	esac
done
shift $((OPTIND - 1))

if ! command -v "${TLC}" >/dev/null 2>&1; then
	echo "check-tla.sh: ${TLC} not found (TLA+ tools: https://github.com/tlaplus/tlaplus)" >&2
	exit 2
fi

configs=("$@")
test ${#configs[@]} -gt 0 || configs=("${MODELS}"/*.cfg)

out="$(mktemp -d)"
trap 'rm -rf "${out}"' EXIT

printf '%-28s %-24s %-24s %s\n' "configuration" "expected" "result" "states"
printf '%-28s %-24s %-24s %s\n' "-------------" "--------" "------" "------"
failed=0
for cfg in "${configs[@]}"; do
	name="$(basename "${cfg}" .cfg)"
	module="${name%%-*}"
	expect="$(sed -n 's/^\\\* EXPECT: *//p' "${cfg}" | head -1)"
	log="${out}/${name}.log"
	(cd "${MODELS}" && "${TLC}" -workers "${jobs}" -metadir "${out}/${name}.meta" \
		-config "$(basename "${cfg}")" "${module}.tla") >"${log}" 2>&1
	status=$?
	states="$(sed -n 's/.* \([0-9,]*\) distinct states found.*/\1/p' "${log}" | tail -1)"
	violated="$(sed -n 's/^Error: Invariant \(.*\) is violated.*/\1/p' "${log}" | head -1)"
	if grep -q 'Temporal properties were violated' "${log}"; then
		result="liveness violated"
	elif test -n "${violated}"; then
		result="${violated} violated"
	elif test "${status}" -eq 0 && grep -q 'No error has been found' "${log}"; then
		result="pass"
	else
		result="error (exit ${status})"
	fi
	if test "${expect}" = "pass"; then
		ok=$(test "${result}" = "pass" && echo 1 || echo 0)
	else
		ok=$(test "${result}" = "${expect} violated" && echo 1 || echo 0)
	fi
	if test "${ok}" -eq 1; then
		printf '%-28s %-24s %-24s %s\n' "${name}" "${expect}" "${result}" "${states}"
	else
		printf '%-28s %-24s %-24s %s   <-- MISMATCH\n' "${name}" "${expect}" "${result}" "${states}"
		grep -E '^Error' "${log}" | head -3 | sed 's/^/    /'
		failed=1
	fi
done
exit "${failed}"
