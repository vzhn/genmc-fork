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

## Verdicts

GenMC decides three of SV-COMP's properties:

| Property | Reported false when GenMC finds |
|---|---|
| `unreach-call` | a safety violation (failed `reach_error`/assertion) |
| `no-data-race` | a non-atomic race |
| `valid-memsafety` | an access to freed or unallocated memory, a double free |

GenMC stops at the first error, so a task is only checked for the property it
fails first. Each task gets one of:

- **AGREE:** GenMC finds the expected error, or finds none when all properties hold.
- **MISSED:** an expected error that GenMC does not report.
- **FALSE-ALARM:** an error on a property that is expected to hold.
- **UNCHECKED:** an error on a property the task does not state.
- Otherwise a label saying why there is no verdict: `TIMEOUT`, `COMPILE-ERROR`,
  `UNSUPPORTED-CALL` (a call to a function GenMC cannot run), `NO-INPUT`
  (the task's input file is missing) and so on.

## What the shim approximates

The preludes make SV-COMP's conventions fit GenMC. Some of them are bounded,
so a clean result holds only up to those bounds:

- **`__VERIFIER_nondet_*`** returns a value in `0..N` (`--nondet-max`, default 2),
  and every value is explored. An execution that asks for more than
  `--max-choices` values (default 4) is discarded, like an unroll bound.
- **Atomic sections** (`__VERIFIER_atomic_begin/end` and functions named
  `__VERIFIER_atomic_*`) take one global lock. The lock orders the sections
  it protects, so it can hide a data race that SV-COMP reports. SV-COMP also
  counts accesses inside atomic sections as atomic; GenMC does not, so a race
  with one side in an atomic section is labelled `ATOMIC-RACE`, with no verdict.
- **`pthread_detach`, mutex attributes and recursive mutexes** are modelled
  in the prelude; GenMC's `pthread.h` lacks them. Preprocessed tasks also
  get rwlocks and thread-specific data in the prelude, modelled as in
  `pthread.h`, for up to 16 threads; a task with more threads gets no verdict. Condition variables and
  `pthread_once` come from GenMC's `pthread.h` (the generated tail of a
  preprocessed task defines them the same way), except that a condvar wait
  may wake up without a signal, at most `--max-spurious` times per
  execution (default 1).
- **`abort()`** and **`malloc(0)`** discard the execution. SV-COMP uses
  `abort()` to cut off paths, and GenMC itself would die on it (#44, #47).

## Results

With GenMC 0.19.0-vzhn.14 and every folder of `Concurrency.set`
(sv-benchmarks of 2026-10-21: 1154 tasks, 1143 with a property GenMC
decides), 60 s per task:

| Verdict | Tasks |
|---|---|
| AGREE | 631 |
| MISSED | 29 |
| FALSE-ALARM | 1 |
| UNCHECKED, ATOMIC-RACE | 16, 40 |
| TIMEOUT | 372 |
| COMPILE-ERROR | 26 |
| UNSUPPORTED-CALL | 17 |
| UNKNOWN, OTHER-ERROR | 6, 5 |

Most timeouts are in `pthread-wmm` (201): its tasks simulate weak memory
with many small atomic sections, and the one lock that implements them
makes GenMC explore every order of the sections. The false alarm is a read
of uninitialised memory, which GenMC reports and SV-COMP does not count.

The missed bugs fall into four groups:
- races hidden by the atomic-section lock;
- bugs that need a specific value (`x == 42`) or integer overflow, beyond the small
  `nondet` range;
- memory leaks, which GenMC does not check;
- out-of-bounds accesses that land in a neighbouring object.

The `UNSUPPORTED-CALL` tasks call library functions that neither GenMC nor
the prelude provides (`sleep`, `time`, `ffs`, `sem_init`, `strcpy`, `mmap`)
or, in one case, a function the task declares but never defines.
