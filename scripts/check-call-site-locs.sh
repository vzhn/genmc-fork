#!/bin/bash

# Checks that the events of the runtime headers' code carry the caller's line
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# The wrappers of GenMC's runtime headers (pthread_mutex_lock(), free(),
# std::atomic, ...) are inlined into the program, or called from it: an
# error report must locate their events at the program's line that calls
# them, and so the block of a thread stuck at a lock. Each test program
# marks those lines with "@<marker>" comments; each expectation below is a
# marker and the event (a regex) that must be reported at its line in the
# graph. No location in the report may name a runtime header. ctest target
# "call-site-locs".
#
#   GenMC=<build>/bin/genmc scripts/check-call-site-locs.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
TESTS="${DIR}/../tests/wrong"
HEADERS="$(ls "${DIR}/../lli/runtime-include/c" | sed 's/\./\\./g' | paste -sd '|')"

status=0

# check <program> <flags> <marker>=<event regex>...
check() {
	local t="$1" flags="$2"
	shift 2
	local name out ok=1
	name="$(basename "${t}")"
	out=$("${GenMC}" ${flags} -disable-mm-detector -disable-estimation -print-error-trace \
		-- "${t}" 2>&1)
	if ! echo "${out}" | grep -q '^Error: '; then
		echo "FAIL ${name} ${flags}: no error reported"
		echo "${out}" | tail -3
		status=1
		return
	fi
	# (graph lines end in "file:line", trace lines are "[fun] file: line: source")
	if echo "${out}" | grep -E "[[:space:]](${HEADERS}):[0-9]+\$|^\[[^]]*\] (${HEADERS}): [0-9]+: "; then
		echo "FAIL ${name} ${flags}: events located in the runtime headers (above)"
		ok=0
	fi
	local e marker event line
	for e in "$@"; do
		marker="${e%%=*}"
		event="${e#*=}"
		line=$(grep -n "@${marker} " "${t}" | cut -d: -f1)
		if ! echo "${out}" | grep -qE "^	${event}.* ${name}:${line}\$"; then
			echo "FAIL ${name} ${flags}: no \"${event}\" at line ${line} (@${marker})"
			ok=0
		fi
	done
	if [[ ${ok} -eq 1 ]]; then
		echo "ok   ${name} ${flags}"
	else
		echo "${out}" | sed -n '/ in graph:$/,/^Coherence:$/p'
		status=1
	fi
}

FREE="${TESTS}/infr/call-site-locs-free/variants/call-site-locs-free.c"
for m in rc11 imm; do
	check "${FREE}" "-${m}" \
		'calloc=\(0, [0-9]+\): MALLOC' \
		'init=\(0, [0-9]+\): Wna' \
		'create-a=\(0, [0-9]+\): THREAD_CREATE \[thread 1\]' \
		'create-b=\(0, [0-9]+\): THREAD_CREATE \[thread 2\]' \
		'lock-g=\(1, [0-9]+\): CRacq \(g\.__private' \
		'lock-g=\(1, [0-9]+\): CWacq \(g\.__private' \
		'unlock-g=\(1, [0-9]+\): Wrel \(g\.__private' \
		'destroy=\(1, [0-9]+\): Wna' \
		'free=\(1, [0-9]+\): FREE' \
		'lock-freed=\(2, [0-9]+\): CRacq'
done

# A thread blocks after its lock's read; under in-place revisiting (the
# default), the block comes from the read's annotation
DEADLOCK="${TESTS}/liveness/call-site-locs-deadlock/variants/call-site-locs-deadlock.c"
for f in "-rc11" "-imm" "-rc11 -disable-ipr"; do
	check "${DEADLOCK}" "${f} -check-liveness" \
		'create-a=\(0, [0-9]+\): THREAD_CREATE \[thread 1\]' \
		'create-b=\(0, [0-9]+\): THREAD_CREATE \[thread 2\]' \
		'a-lock-m1=\(1, [0-9]+\): CRacq \(m1\.__private' \
		'a-lock-m1=\(1, [0-9]+\): CWacq \(m1\.__private' \
		'a-lock-m2=\(1, [0-9]+\): CRacq \(m2\.__private' \
		'a-lock-m2=\(1, [0-9]+\): BLOCK' \
		'b-lock-m2=\(2, [0-9]+\): CRacq \(m2\.__private' \
		'b-lock-m2=\(2, [0-9]+\): CWacq \(m2\.__private' \
		'b-lock-m1=\(2, [0-9]+\): CRacq \(m1\.__private' \
		'b-lock-m1=\(2, [0-9]+\): BLOCK'
done

# Without the function inliner, std::atomic's members stay calls into
# <atomic>: their events are located through the interpreter's stack
JOIN="${TESTS}/infr/call-site-locs-join/variants/call-site-locs-join.cpp"
for f in "-rc11" "-rc11 -disable-function-inliner"; do
	check "${JOIN}" "${f}" \
		'malloc=\(0, [0-9]+\): MALLOC' \
		'create=\(0, [0-9]+\): THREAD_CREATE' \
		'lock=\(0, [0-9]+\): CRacq \(m\.__private' \
		'load=\(0, [0-9]+\): R[a-z]* \(ready' \
		'wait=\(0, [0-9]+\): Racq \(c\.__seq' \
		'wait=\(0, [0-9]+\): Wrel \(m\.__private' \
		'wait=\(0, [0-9]+\): CRacq \(m\.__private' \
		'unlock=\(0, [0-9]+\): Wrel \(m\.__private' \
		'join=\(0, [0-9]+\): THREAD_JOIN \[thread 1\]' \
		'free=\(0, [0-9]+\): FREE' \
		'store=\(1, [0-9]+\): W[a-z]* \(ready' \
		'signal=\(1, [0-9]+\): U?R[a-z]* \(c\.__seq'
done
exit ${status}
