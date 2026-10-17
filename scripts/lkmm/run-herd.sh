#!/bin/bash
#
# Runs herd7 with the Linux-kernel memory model on a set of C litmus tests and
# keeps its output next to the results, one .out file per test.
#
# Usage: run-herd.sh <model-dir> <out-dir> <litmus-root> [<litmus-file>...]
#
# <model-dir> is a copy of the kernel's tools/memory-model (it holds
# linux-kernel.cfg and the .cat/.bell/.def files). Without explicit files, every
# *.litmus under <litmus-root> is run. Each output lands in
# <out-dir>/<path relative to litmus-root>.out; an existing output is kept, so
# an interrupted run can be resumed.
#
# Environment: HERD (default: herd7), HERD_TIMEOUT (default: 60s),
# HERD_JOBS (default: number of CPUs).

set -euo pipefail

if test $# -lt 3; then
	echo "usage: $0 <model-dir> <out-dir> <litmus-root> [<litmus-file>...]" >&2
	exit 2
fi

model=$(realpath "$1")
out=$(realpath -m "$2")
root=$(realpath "$3")
shift 3

HERD="${HERD:-herd7}"
HERD_TIMEOUT="${HERD_TIMEOUT:-60s}"
HERD_JOBS="${HERD_JOBS:-$(getconf _NPROCESSORS_ONLN)}"
export HERD HERD_TIMEOUT model out root

mkdir -p "${out}"

run_one() {
	local t rel o rc
	t=$(realpath "$1")
	rel=${t#"${root}"/}
	o="${out}/${rel%.litmus}.out"
	test -s "${o}" && return 0
	mkdir -p "$(dirname "${o}")"
	rc=0
	(cd "${model}" &&
		timeout "${HERD_TIMEOUT}" "${HERD}" -conf linux-kernel.cfg "${t}") \
		> "${o}.tmp" 2>&1 || rc=$?
	if test "${rc}" -eq 124; then
		echo "TIMEOUT ${HERD_TIMEOUT}" >> "${o}.tmp"
	elif test "${rc}" -ne 0; then
		echo "HERD-EXIT ${rc}" >> "${o}.tmp"
	fi
	mv "${o}.tmp" "${o}"
}
export -f run_one

if test $# -gt 0; then
	printf '%s\n' "$@"
else
	find "${root}" -name '*.litmus' | sort
fi | xargs -r -d '\n' -n 1 -P "${HERD_JOBS}" bash -c 'run_one "$0"'
