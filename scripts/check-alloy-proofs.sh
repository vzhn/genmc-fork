#!/bin/bash
#
# Checks proofs/alloy/rmw-placement.als and that the code agrees with it:
#
#  - for each model (SC, TSO, RA, RC11), the premise of the lemma has an
#    instance (the check is not vacuous), and the lemma either holds within
#    the scope or has a counterexample;
#  - the models with a counterexample are exactly those for which
#    rmwPlacementNeedsCheck() (genmc/genmc/Execution/MemoryModel.hpp) is true;
#  - with KATER_SRC set to a Kater checkout, the Kater models the proof
#    transcribes still have the hashes recorded in
#    proofs/alloy/rmw-placement.kat.sha256 (else: re-check the transcription
#    and update the hashes).
#
# Needs alloy (Alloy 6) on the PATH, or ALLOY; exits 77 (skipped) without it.
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
ROOT="${DIR}/.."
PROOF="${ROOT}/proofs/alloy/rmw-placement.als"
TABLE="${ROOT}/genmc/genmc/Execution/MemoryModel.hpp"
ALLOY="${ALLOY:-$(command -v alloy)}"
SOLVER="${ALLOY_SOLVER:-glucose}"
export ALLOY_JAVA_OPTS="${ALLOY_JAVA_OPTS:--Xss16m -Xmx2g}"

[[ -n "${ALLOY}" ]] || { echo "alloy not found: skipped"; exit 77; }

out=$(mktemp -d)
trap 'rm -rf "${out}"' EXIT
status=0

# Whether COMMAND of the proof has an instance (a counterexample, for a check)
instance() {
	"${ALLOY}" exec -q -s "${SOLVER}" -c "$1" -o "${out}/$1" -t json "${PROOF}" > "${out}/$1.log" 2>&1 ||
		{ echo "FAIL: alloy failed on $1:"; cat "${out}/$1.log"; exit 1; }
	compgen -G "${out}/$1/$1-solution-*.json" > /dev/null
}

# The table's value for MODEL: true or false
table() {
	awk -v m="ModelType::$1:" '/rmwPlacementNeedsCheck/ {in_fn = 1} in_fn && $1 == "case" && $2 == m {getline; gsub(/[;[:space:]]|return/, ""); print; exit}' "${TABLE}"
}

for model in SC TSO RA RC11; do
	instance "premise${model}" ||
		{ echo "FAIL: ${model}: the premise has no instance (the check would be vacuous)"; status=1; continue; }
	if instance "placementKeeps${model}"; then needs=true; else needs=false; fi
	code=$(table "${model}")
	if [[ "${code}" == "${needs}" ]]; then
		echo "ok   ${model}: needs the check: ${needs}"
	else
		echo "FAIL: ${model}: the proof says rmwPlacementNeedsCheck() is ${needs}, the code says '${code}'"
		status=1
	fi
done

if [[ -n "${KATER_SRC:-}" ]]; then
	if (cd "${KATER_SRC}" && grep -v '^#' "${ROOT}/proofs/alloy/rmw-placement.kat.sha256" | sha256sum --quiet -c -); then
		echo "ok   the transcribed Kater models are unchanged"
	else
		echo "FAIL: a Kater model the proof transcribes changed: re-check proofs/alloy/rmw-placement.als and update proofs/alloy/rmw-placement.kat.sha256"
		status=1
	fi
fi
exit ${status}
