#!/bin/bash

# Runs a small set of tests with -check-invariants (see
# genmc/Verification/Checks/Invariants.hpp): the regression tests of the bugs the
# invariants were written for, under the models they run under, plus the
# parallel error reports. ctest target "invariants-fast".
#
#   GenMC=<build>/bin/genmc scripts/check-invariants-fast.sh
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
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
TESTS="${DIR}/../tests"

status=0

# A correct test: every variant explores the expected number of executions
# (the first line of expected.<model>.mo.in, with the first line of
# args.<model>.mo.in), and no invariant fires
correct() {
	local name="$1" model="$2"; shift 2
	local dir="${TESTS}/correct/${name}"
	local args="" expected
	[ -f "${dir}/args.${model}.mo.in" ] && args=$(head -n 1 "${dir}/args.${model}.mo.in" | cut -f1 -d'|')
	expected=$(head -n 1 "${dir}/expected.${model}.mo.in")
	for t in "${dir}"/variants/*.c "${dir}"/variants/*.cpp; do
		[ -f "$t" ] || continue
		out=$("${GenMC}" -check-invariants "-${model}" -disable-mm-detector ${args} "$@" -- "$t" 2>&1)
		explored=$(echo "${out}" | awk '/explored/ { print $6 }')
		if [ "${explored}" = "${expected}" ] && ! echo "${out}" | grep -q "INTERNAL FAILURE"; then
			echo "ok   ${name} $(basename "$t") ${model} $*: ${explored} executions"
		else
			echo "FAIL ${name} $(basename "$t") ${model} $*: ${explored:-no count}, expected ${expected}"
			echo "${out}" | grep -E "Invariant|INTERNAL FAILURE|Error" | head -3
			status=1
		fi
	done
}

# A wrong test: every variant reports the error, and no invariant fires
wrong() {
	local name="$1" model="$2" error="$3"; shift 3
	local dir="${TESTS}/wrong/${name}"
	local args=""
	[ -f "${dir}/genmc.${model}.mo.in" ] && args=$(head -n 1 "${dir}/genmc.${model}.mo.in")
	for t in "${dir}"/variants/*.c; do
		out=$("${GenMC}" -check-invariants "-${model}" -disable-mm-detector ${args} "$@" -- "$t" 2>&1)
		if echo "${out}" | grep -q "${error}" && ! echo "${out}" | grep -q "INTERNAL FAILURE"; then
			echo "ok   ${name} $(basename "$t") ${model} $*: ${error}"
		else
			echo "FAIL ${name} $(basename "$t") ${model} $*: expected '${error}'"
			echo "${out}" | grep -E "Invariant|INTERNAL FAILURE|Error|No errors" | head -3
			status=1
		fi
	done
}

for model in rc11 imm sc; do
	# I1: restriction (PR #32)
	correct infr/nested-create-revisit "${model}"
	correct infr/nested-create-erase "${model}"
	correct infr/nested-create-symm "${model}"
	# I2: kills during replays (PR #31)
	correct infr/kill-error-replay "${model}"
	correct infr/kill-replay-norace "${model}"
	# I3: zero-size blocks across executions (issue #115)
	correct infr/malloc-zero-revisit "${model}"
done
# I3: allocations rebuilt at their addresses (PR #66)
correct gobench/cockroach10790 imm
# I4: resumed executions driven through holes (issue #111)
correct infr/imm-resume-holes imm
correct infr/imm-resume-holes rc11
# I1 on graphs handed to other workers
correct infr/imm-parallel-copy imm -nthreads=2
# I6: an error inside an open read-side critical section is checked with the
# section closed (issue #201)
correct lkmm/rcu-assert-inside lkmm
correct lkmm/srcu-assert-inside lkmm
# I5: coherence; an RMW read filled into a hole before a kept read of its
# location reads no write that read reads
correct lkmm/fai-hole-kept-read lkmm
# I6: an error found as a read is added is checked without the read, which
# has no rf yet (issue #221)
for model in sc rc11 imm; do
	correct infr/ipr-error-pending-read "${model}"
done
for model in rc11 imm; do
	# I2: the replay target stays in the schedule (PR #22)
	wrong liveness/spin-na-stuck "${model}" "Liveness violation"
	wrong liveness/await-stuck-join "${model}" "Liveness violation"
	# I2: no stale replay across the tasks of a worker (PR #19)
	wrong safety/parallel-error-report "${model}" "Safety violation" -nthreads=4
done
# I5 under TSO: the checker's whole-graph check walks plain writes; a racing
# plain write is not in coherence
correct infr/tso-plain-write-check tso
wrong racy/MPU+rel+rlx tso "Non-atomic race"
exit ${status}
