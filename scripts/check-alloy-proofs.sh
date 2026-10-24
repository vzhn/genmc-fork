#!/bin/bash
#
# Checks the Alloy proofs in proofs/alloy and that the code agrees with them:
#
#  - proofs/alloy/rmw-placement.als: for each model (SC, TSO, RA, RC11), the
#    premise of the lemma has an instance (the check is not vacuous), and the
#    lemma either holds within the scope or has a counterexample; the models
#    with a counterexample are exactly those for which
#    rmwPlacementNeedsCheck() (genmc/genmc/Execution/MemoryModel.hpp) is true;
#  - proofs/alloy/open-section.als: the same for SC, TSO, RA, RC11 and LKMM
#    and openSectionsNeedCheck();
#  - with KATER_SRC set to a Kater checkout, the Kater models each proof
#    transcribes still have the hashes recorded in proofs/alloy/<proof>.kat.sha256
#    (else: re-check the transcription and update the hashes).
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
PROOFS="${ROOT}/proofs/alloy"
TABLE="${ROOT}/genmc/genmc/Execution/MemoryModel.hpp"
ALLOY="${ALLOY:-$(command -v alloy)}"
SOLVER="${ALLOY_SOLVER:-glucose}"
export ALLOY_JAVA_OPTS="${ALLOY_JAVA_OPTS:--Xss16m -Xmx2g}"

[[ -n "${ALLOY}" ]] || { echo "alloy not found: skipped"; exit 77; }

out=$(mktemp -d)
trap 'rm -rf "${out}"' EXIT
status=0

# Whether COMMAND of PROOF has an instance (a counterexample, for a check)
instance() { # <proof.als> <command>
	local name
	name="$(basename "$1" .als)-$2"
	"${ALLOY}" exec -q -s "${SOLVER}" -c "$2" -o "${out}/${name}" -t json "$1" > "${out}/${name}.log" 2>&1 ||
		{ echo "FAIL: alloy failed on $2 of $(basename "$1"):"; cat "${out}/${name}.log"; exit 1; }
	compgen -G "${out}/${name}/$2-solution-*.json" > /dev/null
}

# The value of the table function FN for MODEL: true or false
table() { # <fn> <MODEL>
	awk -v fn="$1" -v m="ModelType::$2:" '$0 ~ fn {in_fn = 1} in_fn && $1 == "case" && $2 == m {getline; gsub(/[;[:space:]]|return/, ""); print; exit}' "${TABLE}"
}

# A proof: each model's premise has an instance, and the table says whether
# the lemma has a counterexample
check_proof() { # <proof> <table fn> <lemma prefix> <MODEL>...
	local name="$1" proof="${PROOFS}/$1.als" fn="$2" lemma="$3" model
	shift 3
	echo "${name}:"
	for model in "$@"; do
		instance "${proof}" "premise${model}" ||
			{ echo "FAIL: ${model}: the premise has no instance (the check would be vacuous)"; status=1; continue; }
		if instance "${proof}" "${lemma}${model}"; then needs=true; else needs=false; fi
		code=$(table "${fn}" "${model}")
		if [[ "${code}" == "${needs}" ]]; then
			echo "ok   ${model}: needs the check: ${needs}"
		else
			echo "FAIL: ${model}: the proof says ${fn}() is ${needs}, the code says '${code}'"
			status=1
		fi
	done
	if [[ -n "${KATER_SRC:-}" ]]; then
		if (cd "${KATER_SRC}" && grep -v '^#' "${PROOFS}/${name}.kat.sha256" | sha256sum --quiet -c -); then
			echo "ok   the transcribed Kater models are unchanged"
		else
			echo "FAIL: a Kater model the proof transcribes changed: re-check ${proof} and update ${PROOFS}/${name}.kat.sha256"
			status=1
		fi
	fi
}

check_proof rmw-placement rmwPlacementNeedsCheck placementKeeps SC TSO RA RC11
check_proof open-section openSectionsNeedCheck closingKeeps SC TSO RA RC11 LKMM
exit ${status}
