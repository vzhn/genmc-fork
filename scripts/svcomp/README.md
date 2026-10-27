# SV-COMP concurrency tasks on GenMC

Runs GenMC on the concurrency tasks of the
[SV-COMP benchmarks](https://gitlab.com/sosy-lab/benchmarking/sv-benchmarks)
and compares each result with the task's expected verdict. The tasks are not
part of this repository; fetch them first.

| File | What it does |
|---|---|
| `svcomp-run.py` | runs GenMC on every task of the given folders, writes `results.tsv` and one `.out` per task |
| `svcomp-shim.h` | prelude for tasks that include `<pthread.h>` |
| `svcomp-shim-pp.h` | prelude for preprocessed tasks, which declare the pthread API themselves |
| `svcomp-shim-core.h` | the part both preludes share: nondeterminism, atomic sections, `main` |
| `selftest/` | small tasks that `scripts/check-svcomp-shim.sh` (ctest `svcomp-shim`) runs through both preludes |

## Usage

Fetch the C tasks (about 100 MB for the concurrency folders):

    git clone --filter=blob:none --sparse \
        https://gitlab.com/sosy-lab/benchmarking/sv-benchmarks.git
    cd sv-benchmarks
    git sparse-checkout set c/properties c/pthread c/pthread-ext c/weaver ...

Then run:

    scripts/svcomp/svcomp-run.py -j 12 -t 60 -o results build/bin/genmc \
        sv-benchmarks/c/pthread sv-benchmarks/c/weaver ...

Like SV-COMP, it verifies each task's `input_files`: for most tasks the
preprocessed `.i`, which unlike the `.c` it was made from needs no headers
from outside the task. It needs Python 3 with PyYAML, and a GenMC whose
`pthread.h` has `pthread_once` (0.19.0-vzhn.14 or later). Every task is run under `-rc11`
(`-m` picks another model). The tasks are SC programs, so a race or
assertion failure found under RC11 also exists under SC, unless it depends
on weak behaviour of the task's atomics.

With `--per-property`, each property of a task is checked alone, as SV-COMP
does, with a verdict per task/property pair (see below).

## The SV-COMP wrapper

SV-COMP runs a verifier on one property of one task. `genmc-svcomp` does
that with GenMC:

    genmc-svcomp [--genmc GENMC] [--data-model ILP32|LP64] \
        [--witness-format yaml|graphml] [--timeout SECONDS] PROPERTY_FILE PROGRAM

It prints GenMC's output, then SV-COMP's result as the last line:
`SV-COMP result: true`, `false(unreach-call)`, `false(no-data-race)`,
`false(valid-deref)`, `false(valid-free)` or `unknown`. For a false result
it writes the violation witness to `witness.yml` (format 2.2) or
`witness.graphml` (1.0) in the working directory. `benchexec/genmc.py` is
the tool-info module through which BenchExec runs it. GenMC's runs share
`--timeout` seconds (default 800, within SV-COMP's 900 s of CPU time): the
runs with more nondet values above take what the first run leaves.

Only the property checked counts. An error of another property gives
`unknown`, and so does a bound that cut executions off. `reach_error()`
ends an execution, instead of failing, unless `unreach-call` is checked, and
`malloc()` may fail only if `valid-memsafety` is. A clean `valid-memsafety`
run that leaves memory unfreed is `unknown` as well: GenMC does not tell
whether the memory is still reachable (`valid-memtrack`).
`svcomp-run.py --per-property` checks each property the same way.

## Verdicts

GenMC decides three of SV-COMP's properties:

| Property | Reported false when GenMC finds |
|---|---|
| `unreach-call` | a safety violation (failed `reach_error`/assertion) |
| `no-data-race` | a non-atomic race |
| `valid-memsafety` | an access to freed or unallocated memory, a double free |

The runner passes GenMC `-svcomp` where it has the option: a read of
uninitialized memory, which SV-COMP does not count as a memory-safety
violation, then reads 0 instead of being reported.

GenMC stops at the first error, so a task is only checked for the property it
fails first. Each task gets one of:

- **AGREE:** GenMC finds the expected error, or finds none when all properties hold.
- **MISSED:** an expected error that GenMC does not report.
- **FALSE-ALARM:** an error on a property that is expected to hold.
- **UNCHECKED:** an error on a property the task does not state.
- Otherwise a label saying why there is no verdict: `TIMEOUT`, `COMPILE-ERROR`,
  `UNSUPPORTED-CALL` (a call to a function GenMC cannot run), `NO-INPUT`
  (the task's input file is missing), `CREATE-RACE` and `BOUNDED` (see
  below) and so on.

## What the shim approximates

The preludes make SV-COMP's conventions fit GenMC. Some of them are bounded,
so a clean result holds only up to those bounds:

- **`__VERIFIER_nondet_*`** returns a value in `0..N` (`--nondet-max`, default 2),
  and every value is explored. An execution that asks for more than
  `--max-choices` values (default 4) is discarded, like an unroll bound.
  A clean result with discarded (blocked) executions is checked again: if
  an execution asked for more values, or aborted (`assume_abort_if_not`)
  in a thread that had read the largest value, as when a task wants
  `n >= 20`, the bounds may have cut an error off, and the task is
  labelled `BOUNDED`, with no verdict.
  A task that calls an integer `__VERIFIER_nondet_*` and gets a clean or
  `BOUNDED` result is then run again for each of a few more values, in a
  share of the time left: 3, 4, -1, the constants the task compares with
  and their neighbours, its `case` labels, 2^30+1 (a size that wraps around
  when multiplied by 4 in 32 bits) and `INT_MAX`. In each run the nondet
  calls return 0..N or that value, and calls past `--max-choices` return 0
  instead of ending the execution. An error found so is reported: each
  value is one the type can hold. These runs only look for errors, so they
  leave a result without one as it was.
- **Atomic sections** (`__VERIFIER_atomic_begin/end` and functions named
  `__VERIFIER_atomic_*`) take one global lock. The lock orders the sections
  it protects, so it can hide a data race that SV-COMP reports. SV-COMP also
  counts accesses inside atomic sections as atomic; GenMC does not, so a race
  with one side in an atomic section is labelled `ATOMIC-RACE`, with no verdict.
- **`pthread_detach`, mutex attributes and recursive mutexes** are modelled
  in the prelude; GenMC's `pthread.h` lacks them. Preprocessed tasks also
  get rwlocks and thread-specific data in the prelude, modelled as in
  `pthread.h` but with key destructors, which a thread that returns or
  calls `pthread_exit` runs, for up to 16 threads; a task with more threads gets no verdict. Condition variables and
  `pthread_once` come from GenMC's `pthread.h` (the generated tail of a
  preprocessed task defines them the same way), except that a condvar wait
  may wake up without a signal, at most `--max-spurious` times per
  execution (default 1).
- **`pthread_create`** stores the new thread's ID after creating the
  thread; glibc stores it before the thread runs. A thread that reads the ID
  once the new thread has signalled it races with the store only in GenMC,
  so a race on that store is labelled `CREATE-RACE`, with no verdict. The
  prelude's `pthread_create` stores the ID atomically, so that the runner
  can tell that store from one of the task's own.
- **`abort()`** and **`malloc(0)`** discard the execution. SV-COMP uses
  `abort()` to cut off paths, and GenMC itself would die on it (#44, #47).
- **`malloc` and `calloc` may fail**, as in SV-COMP, for tasks that check
  `valid-memsafety`: in each execution, at most one allocation returns NULL.
- **Library functions** that GenMC lacks: `sleep` and `usleep` return at
  once, `strcpy`, `ffs` and POSIX semaphores (`sem_init`, `sem_wait`,
  `sem_trywait`, `sem_post`, ...) are defined in the generated tail of a
  preprocessed task, and `scanf(fmt, p)` stores a nondet value through `p`.
  `time` is not modelled (a clock that a task loops on until a deadline
  cannot be bounded soundly), nor is `mmap` (its task forks).
- **gcc's leniency:** SV-COMP's tasks are written for gcc, which only warns
  about passing a pointer as an int (`assume_abort_if_not(p)`); the runner
  makes clang warn too.

## Witnesses

With `--witness`, the runner writes a violation witness for each error GenMC
finds, `<task>.witness.yml` (format 2.2, the first with threads) and
`<task>.witness.graphml` (format 1.0 with `threadId`/`createThread`, SV-COMP
2026's format for concurrency). A witness gives the thread creations, in an
order the execution has, and the error: the call of `reach_error()`, the
two accesses of a race, or the invalid access. The validator finds the rest
of the interleaving. GenMC runs with `-library-dir`, so that the code the
runner adds (the prelude, the generated tail and `reach_error()`) is
reported at the task's line that called it: a witness may only name lines
of the task.

Of the 306 errors GenMC finds in `Concurrency.set`, the SV-COMP 2026
validators confirm 235 witnesses within SV-COMP's 90 s (Dartagnan and
CPAchecker on GraphML, UAutomizer on YAML; one is enough): 111 of 136
`unreach-call`, 116 of 162 `no-data-race`, 8 of 8 `valid-memsafety`. In 20
cases all three time out; in most others UAutomizer finds no violation
along the witness, or Dartagnan cannot parse the task.

## Results

With GenMC built from master (after 0.19.0-vzhn.14, with `-svcomp`) and
every folder of `Concurrency.set` (sv-benchmarks of 2026-10-26: 1154 tasks,
1143 with a property GenMC decides), 60 s per task:

| Verdict | Tasks |
|---|---|
| AGREE | 491 |
| MISSED | 8 |
| FALSE-ALARM | 0 |
| BOUNDED | 212 |
| UNCHECKED, ATOMIC-RACE, CREATE-RACE | 6, 40, 5 |
| TIMEOUT | 369 |
| COMPILE-ERROR | 0 |
| UNSUPPORTED-CALL | 4 |
| UNKNOWN, OTHER-ERROR | 3, 5 |

Most timeouts are in `pthread-wmm` (201): its tasks simulate weak memory
with many small atomic sections, and the one lock that implements them
makes GenMC explore every order of the sections.

Almost every `BOUNDED` task has no complete execution at all: each
execution asks for more nondet values than `--max-choices` allows, or wants
a value beyond the range. Before the check, 21 of them were bugs reported
"true", 168 were "true" only by luck, and one timed out; a larger
`--max-choices` completes few of them (with 8, 9 more AGREE; with 16, 11,
and many more timeouts). The other 22 are `pthread-driver-races`, whose
harnesses ask for some fifty nondet values.

The missed bugs need a value beyond the small `nondet` range that no
`assume` asks for (an index that reaches a neighbouring object, as in
the `goblint-regression` ones), or a race hidden by the atomic-section
lock; GenMC also does not check memory leaks.

The `UNSUPPORTED-CALL` tasks call `time` or `mmap`, which the prelude does
not model, or, in one case, a function the task declares but never
defines.
