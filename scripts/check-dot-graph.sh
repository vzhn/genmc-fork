#!/bin/bash

# Checks the error graph that -dump-error-graph writes in DOT
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# For a race, the graph has a cluster per thread, highlights both racing
# events, and draws the rf edge between them; for an assertion after
# joins, it highlights the failing event and draws the joins. Graphviz
# must accept the file (when installed). A file that cannot be written is
# a user error. ctest target "dot-graph".
#
#   GenMC=<build>/bin/genmc scripts/check-dot-graph.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
WRONG="${DIR}/../tests/wrong"
work=$(mktemp -d)
trap 'rm -rf "${work}"' EXIT

status=0
fail() { echo "FAIL $1"; status=1; }

# dump <name> <program>: writes ${work}/<name>.dot, which must be an error graph
dump() {
	"${GenMC}" -rc11 -disable-estimation -dump-error-graph="${work}/$1.dot" -- "$2" > "${work}/$1.out" 2>&1
	local rc=$?
	[[ ${rc} -eq 42 ]] || { fail "$1: status ${rc}, expected 42 (an error)"; tail -3 "${work}/$1.out"; return 1; }
	[[ -s "${work}/$1.dot" ]] || { fail "$1: no graph written"; return 1; }
	head -1 "${work}/$1.dot" | grep -qx 'strict digraph {' || fail "$1: not a digraph"
	tail -1 "${work}/$1.dot" | grep -qx '}' || fail "$1: the graph is not closed"
	if command -v dot > /dev/null; then
		dot -Tsvg "${work}/$1.dot" -o /dev/null 2> "${work}/$1.dot.err" ||
			{ fail "$1: Graphviz rejects the graph"; cat "${work}/$1.dot.err"; }
	fi
	return 0
}
count() { grep -c -- "$2" "${work}/$1.dot"; }

# A race between a non-atomic write and read: main and two threads
if dump race "${WRONG}/racy/cas-failord-mp/variants/cas-failord-mp0.c"; then
	[[ $(count race '^subgraph cluster_') -eq 3 ]] || fail "race: $(count race '^subgraph cluster_') thread clusters, expected 3"
	[[ $(count race 'fillcolor=yellow') -eq 2 ]] || fail "race: $(count race 'fillcolor=yellow') highlighted events, expected 2"
	grep -q '"(1, 1)"->"(2, 2)"\[color=green' "${work}/race.dot" ||
		fail "race: no rf edge from the racing write (1, 1) to the read (2, 2)"
	grep -q 'cas-failord-mp0.c:24' "${work}/race.dot" || fail "race: no source location"
	echo "ok   race"
fi

# An assertion after joins
if dump assert "${WRONG}/safety/assert-message/variants/assert-message.c"; then
	[[ $(count assert 'fillcolor=yellow') -eq 1 ]] || fail "assert: $(count assert 'fillcolor=yellow') highlighted events, expected 1"
	grep -q 'THREAD_JOIN' "${work}/assert.dot" || fail "assert: no join"
	[[ $(count assert 'color=blue') -ge 4 ]] || fail "assert: $(count assert 'color=blue') create/join edges, expected 4"
	echo "ok   assert"
fi

out=$("${GenMC}" -rc11 -disable-estimation -dump-error-graph="${work}/no/such/dir/x.dot" -- \
	"${WRONG}/racy/cas-failord-mp/variants/cas-failord-mp0.c" 2>&1)
rc=$?
if [[ ${rc} -eq 17 ]] && echo "${out}" | grep -q 'Failed to open dot file'; then
	echo "ok   unwritable file"
else
	fail "unwritable file: status ${rc}, expected 17 and \"Failed to open dot file\""
fi
exit ${status}
