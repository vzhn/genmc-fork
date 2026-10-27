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
#   a location);
# - bounded-*: errors beyond the shim's bounds (a fifth nondet value, a
#   value of at least 20): every execution that could reach them is cut,
#   and the bounds check must label them BOUNDED instead of "true";
#   blocked-not-bounded: executions blocked for the task's own reasons,
#   whose clean result stands;
# - pp-libc, pp-sem-race: sleep, strcpy, ffs and semaphores in the tail (a
#   semaphore of 1 orders the workers, one of 2 does not); scanf: scanf in
#   a task that includes <stdio.h>;
# - pp-key-dtors, pp-key-dtor-error: key destructors, which a thread that
#   returns or calls pthread_exit runs on its non-NULL values;
# - pp-int-pthread-t: a preprocessed task that declares pthread_t as int,
#   as pthread-driver-races does;
# - malloc-fail, malloc-fail-checked: malloc may return NULL; the task that
#   checks it passes the pointer as an int, which clang must only warn about;
#   malloc-fail-after-nondet: its abort on a failed malloc, after reading
#   the largest nondet value, is no bound hit.
# The runner writes violation witnesses (--witness); the waypoints of five
# are checked: a race in a task that includes <pthread.h> and in a
# preprocessed one (both accesses at the task's lines, the code the runner
# adds reported at its caller's), a call of reach_error() by threads created
# in a loop, a NULL dereference after a failed malloc (valid-deref), and a
# call of reach_error() in a key destructor, whose thread the shim starts
# through a function of its own: the witness names the task's. The
# SV-COMP wrapper genmc-svcomp is run on one property of four tasks.
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

"${SVCOMP}/svcomp-run.py" -j 4 -t 60 --witness -o "${out}" "${GenMC}" "${TASKS}" > /dev/null || exit 1

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
	bounded-*) want=BOUNDED ;;
	esac
	if [ "${v}" = "${want}" ]; then
		echo "ok   ${name}: ${got}"
	else
		echo "FAIL ${name}: ${v}, not ${want} (expected ${exp})"; head -10 "${out}/${d}/${name}.out"; status=1
	fi
done < "${out}/results.tsv"
# The SV-COMP wrapper, on one property of a task: SV-COMP's result as its
# last line, and witness.yml for a false one; an error of another property
# is unknown
props="${out}/properties"
mkdir -p "${props}"
echo 'CHECK( init(main()), LTL(G ! call(reach_error())) )' > "${props}/unreach-call.prp"
echo 'CHECK( init(main()), LTL(G ! data-race) )' > "${props}/no-data-race.prp"
printf '%s\n' 'CHECK( init(main()), LTL(G valid-free) )' 'CHECK( init(main()), LTL(G valid-deref) )' \
	'CHECK( init(main()), LTL(G valid-memtrack) )' > "${props}/valid-memsafety.prp"
for t in "create-then-write.c no-data-race false(no-data-race) witness.yml" \
	 "malloc-fail.c valid-memsafety false(valid-deref) witness.yml" \
	 "create-then-write.c unreach-call unknown -" \
	 "once.c unreach-call true -"; do
	set -- ${t}
	wd="${out}/wrapper-${1%.*}-$2"
	mkdir -p "${wd}"
	res=$(cd "${wd}" && "${SVCOMP}/genmc-svcomp" --genmc "${GenMC}" "${props}/$2.prp" \
		"${TASKS}/$1" | tail -1)
	if [ "${res}" = "SV-COMP result: $3" ] && { [ "$4" = - ] || [ -f "${wd}/$4" ]; }; then
		echo "ok   genmc-svcomp $1 $2: $3"
	else
		echo "FAIL genmc-svcomp $1 $2: ${res}, not $3 (witness $4: $(ls "${wd}"))"; status=1
	fi
done

# Each witness's specification and waypoints: (type, thread, line, column)
python3 - "${out}/selftest" <<'PY' || status=1
import sys, yaml
expected = {
    "create-then-write": ("G ! data-race", [("function_enter", 0, 23, 33),
                                            ("target", 1, 15, 2), ("target", 0, 24, 2)]),
    "pp-create-then-write": ("G ! data-race", [("function_enter", 0, 31, 33),
                                               ("target", 1, 23, 2), ("target", 0, 32, 2)]),
    "once-reachable": ("G ! call(reach_error())", [("function_enter", 0, 31, 46),
                                                    ("function_enter", 0, 31, 46),
                                                    ("target", 0, 35, 3)]),
    "malloc-fail": ("G valid-deref", [("target", 0, 14, 2)]),
    "pp-key-dtor-error": ("G ! call(reach_error())", [("function_enter", 0, 71, 36),
                                                       ("target", 1, 56, 3)]),
}
# the function of each target waypoint, where it matters
functions = {"pp-key-dtor-error": ["worker"]}
ok = True
for name, (spec, waypoints) in expected.items():
    try:
        w = yaml.safe_load(open(f"{sys.argv[1]}/{name}.witness.yml"))[0]
    except OSError:
        print(f"FAIL {name}: no witness"); ok = False; continue
    got = [(x["waypoint"]["type"], x["waypoint"]["thread_id"], x["waypoint"]["location"]["line"],
            x["waypoint"]["location"].get("column")) for s in w["content"] for x in s["segment"]]
    funcs = [x["waypoint"]["location"].get("function") for s in w["content"] for x in s["segment"]
             if x["waypoint"]["type"] == "target"]
    if name in functions and funcs != functions[name]:
        print(f"FAIL {name} witness: target functions {funcs}"); ok = False; continue
    if spec not in w["metadata"]["task"]["specification"] or got != waypoints:
        print(f"FAIL {name} witness: {w['metadata']['task']['specification']} {got}"); ok = False
    else:
        print(f"ok   {name} witness")
sys.exit(0 if ok else 1)
PY
exit ${status}
