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

## Usage

Fetch the C tasks (about 100 MB for the concurrency folders):

    git clone --filter=blob:none --sparse \
        https://gitlab.com/sosy-lab/benchmarking/sv-benchmarks.git
    cd sv-benchmarks
    git sparse-checkout set c/properties c/pthread c/pthread-ext c/weaver ...

Then run:

    scripts/svcomp/svcomp-run.py -j 12 -t 60 -o results build/bin/genmc \
        sv-benchmarks/c/pthread sv-benchmarks/c/weaver ...

It needs Python 3 with PyYAML. Every task is run under `-rc11` (`-m` picks
another model). The tasks are SC programs, so a race or assertion failure
found under RC11 also exists under SC, unless it depends on weak behaviour of
the task's atomics.

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
  `UNSUPPORTED-CALL` (a call to a function GenMC cannot run), `NO-C-SOURCE`
  (the task ships only a `.i` file) and so on.

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
- **Condition variables**, `pthread_once`, `pthread_detach`, mutex attributes
  and recursive mutexes are modelled in the prelude; GenMC's `pthread.h` lacks
  them. A condvar wait may wake up without a signal, at most
  `--max-spurious` times per execution (default 1).
- **`abort()`** and **`malloc(0)`** discard the execution. SV-COMP uses
  `abort()` to cut off paths, and GenMC itself would die on it (#44, #47).

## Results

With GenMC 0.19.0-vzhn.5, the folders of `Concurrency.set` except
`pthread-wmm` (891 tasks), 60 s per task:

| Verdict | Tasks |
|---|---|
| AGREE | 528 |
| MISSED | 26 |
| FALSE-ALARM | 0 |
| UNCHECKED, ATOMIC-RACE | 12, 4 |
| TIMEOUT | 87 |
| COMPILE-ERROR | 74 |
| UNSUPPORTED-CALL | 58 |
| NO-C-SOURCE | 32 |
| UNKNOWN, OTHER-ERROR | 34, 5 |

The missed bugs fall into four groups:
- races hidden by the atomic-section lock;
- bugs that need a specific value (`x == 42`) or integer overflow, beyond the small
  `nondet` range;
- memory leaks, which GenMC does not check;
- out-of-bounds accesses that land in a neighbouring object.

Most `UNSUPPORTED-CALL` tasks (in `pthread-ext`) call functions they declare but
never define.
