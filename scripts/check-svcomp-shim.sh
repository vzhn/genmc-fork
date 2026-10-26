#!/bin/bash

# Checks that the SV-COMP preludes in scripts/svcomp build and give the right verdicts
#
# This program is dual-licensed under the Apache License 2.0 and the MIT License.
# You may choose to use, distribute, or modify this software under either license.
#
# Apache License 2.0:
#     http://www.apache.org/licenses/LICENSE-2.0
#
# MIT License:
#     https://opensource.org/licenses/MIT

# svcomp-shim.h is included before tasks that include <pthread.h>, so it
# must not define what GenMC's pthread.h defines; once pthread.h got
# pthread_once, the shim's own copy stopped every such task from compiling.
# Runs svcomp-run.py on the tasks in scripts/svcomp/selftest and checks
# that every verdict is AGREE, except where noted:
# - once*: pthread_once with <pthread.h> (with and without the shim's
#   recursive mutexes) and in a preprocessed task (svcomp-shim-pp.h plus
#   the generated tail);
# - input-file: the input file is a .i made from a .c that does not build,
#   so the runner must verify the input file, as SV-COMP does;
# - pp-*: preprocessed tasks that check calls in if (...) { and use
#   recursive mutexes, rwlocks and thread-specific data, which the
#   generated tail must define;
# - uninit-read: a read of uninitialized memory, which SV-COMP does not
#   count as a memory-safety violation (GenMC runs with -svcomp);
# - create-order, pp-create-order: a thread reads another's ID once that
#   thread has signalled. glibc's pthread_create stores the ID before the
#   thread runs, GenMC's after, so GenMC reports a race that SV-COMP does
#   not have: the runner must label it CREATE-RACE, with no verdict;
# - create-then-write, pp-create-then-write: the task's own store of the
#   same value right after pthread_create is a race SV-COMP has too, not a
#   CREATE-RACE (in the preprocessed task, GenMC prints that store without
#   a location).
# Needs Python 3 with PyYAML; skipped (exit 77) without it. ctest target
# "svcomp-shim".
#
#   GenMC=<build>/bin/genmc scripts/check-svcomp-shim.sh

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
GenMC="${GenMC:-$DIR/../RelWithDebInfo/bin/genmc}"
SVCOMP="${DIR}/svcomp"
TASKS="${SVCOMP}/selftest"

python3 -c 'import yaml' 2>/dev/null || { echo "SKIP no PyYAML for python3"; exit 77; }

out=$(mktemp -d)
trap 'rm -rf "${out}"' EXIT

"${SVCOMP}/svcomp-run.py" -j 4 -t 60 -o "${out}" "${GenMC}" "${TASKS}" > /dev/null || exit 1

status=0
ntasks=$(ls "${TASKS}"/*.yml | wc -l)
nrows=$(wc -l < "${out}/results.tsv")
if [ "${nrows}" -ne "${ntasks}" ]; then
	echo "FAIL ${nrows} results for ${ntasks} tasks"; status=1
fi
while IFS=$'\t' read -r d name exp got dt v first; do
	want=AGREE
	case "${name}" in
	*create-order) want=CREATE-RACE ;;
	esac
	if [ "${v}" = "${want}" ]; then
		echo "ok   ${name}: ${got}"
	else
		echo "FAIL ${name}: ${v}, not ${want} (expected ${exp})"; head -10 "${out}/${d}/${name}.out"; status=1
	fi
done < "${out}/results.tsv"
exit ${status}
