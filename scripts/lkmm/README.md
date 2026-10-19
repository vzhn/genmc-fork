# LKMM tooling

Tools for adding the Linux-kernel memory model (LKMM) to GenMC and for
checking it against herd7, the reference implementation of LKMM.

| Tool | What it does |
|---|---|
| `run-herd.sh` | runs herd7 with LKMM on C litmus tests, one `.out` per test |
| `litmus2c.py` | translates a C litmus test into a GenMC program |
| `lkmm-diff.py` | runs GenMC on the translations and compares with herd7 |
| `lkmm-c11.h` | kernel primitives mapped onto C11 atomics (an approximation, for `-imm`) |
| `classify.py` | sorts the disagreements of a run by likely cause |
| `kater-export.sh` | regenerates GenMC's consistency checkers from the kat files |
| `compare-checkers.sh` | compares two GenMC builds under every model |

## Oracles (pinned)

- **Kernel model**: `tools/memory-model` of Linux 6.18.54 (`variant lkmmv2`),
  copied out of the source tree (the kernel scripts write next to it):

      cp -r /usr/src/linux-6.18.54-gentoo/tools/memory-model ~/.local/src/lkmm-oracle/memory-model-6.18.54

- **herd7** 7.58, in its own opam switch:

      opam switch create lkmm ocaml-base-compiler.5.3.0 --no-switch
      opam install --switch=lkmm herdtools7

  Run it as `opam exec --switch=lkmm -- herd7 ...`.
- **Litmus corpus**: <https://github.com/paulmckrcu/litmus> at 2f1be9ed
  (5571 tests: 5388 C, 174 LISA, 9 PPC).

## herd7 reference results

    M=~/.local/src/lkmm-oracle/memory-model-6.18.54
    opam exec --switch=lkmm -- scripts/lkmm/run-herd.sh $M ~/.local/src/lkmm-oracle/herd-kernel $M/litmus-tests
    HERD_JOBS=12 HERD_TIMEOUT=60s opam exec --switch=lkmm -- \
        scripts/lkmm/run-herd.sh $M ~/.local/src/lkmm-oracle/herd-corpus ~/.local/src/litmus

The kernel's own check (`scripts/checkalllitmus.sh` in the model directory)
reproduces all 36 of its tests' `Result:` lines. On the corpus, 117 of the
5571 tests do not finish within 60 s; the corpus takes about 2.5 CPU-hours.

**What is compared.** herd7's `States` list is the set of final values of
the condition's observables (plus `locations [...]`) over all allowed
executions; `lkmm-diff.py` compares it with the same projection of GenMC's
executions. herd7's `Positive + Negative` is the number of allowed
executions, and it counts the same thing as GenMC's "complete executions
explored" (consistent rf/co choices): on the 29 kernel tests where the
states agree, the counts agree too. For a test herd7 flags with
`data-race`, the states are undefined (as in C11), and only the race
verdict is compared.

## Translating litmus tests

    scripts/lkmm/litmus2c.py --header $PWD/scripts/lkmm/lkmm-c11.h test.litmus -o test.c

Locations become globals, registers per-thread globals; `main` joins the
threads and writes every observable to one variable after a
`__VERIFIER_output()` marker, so `-print-exec-graphs` shows each execution's
final state. Addresses are written as sentinels, so a state can say that a
register points to `x`, as herd7 does. Unsupported constructs (SRCU,
`spin_is_locked()`, loops, arrays, ...) are reported, never translated
approximately.

## Differential runs

    O=~/.local/src/lkmm-oracle
    scripts/lkmm/lkmm-diff.py --herd-out $O/herd-kernel --litmus-root $O/memory-model-6.18.54/litmus-tests \
        $O/memory-model-6.18.54/litmus-tests
    scripts/lkmm/lkmm-diff.py --herd-out $O/herd-corpus --litmus-root ~/.local/src/litmus \
        --jobs 12 --json corpus.jsonl ~/.local/src/litmus

Results per test: `agree`, `agree-racy`, `missing` (GenMC lacks herd7
states: stronger), `extra` (GenMC has states herd7 forbids: weaker),
`differ`, `race`, `count`, `skipped:<reason>`, timeouts and errors, with a
summary table per directory. `--allowlist` takes classified disagreements;
anything else that disagrees makes the exit status 1. `classify.py` groups
the disagreements of a `--json` file by likely cause.

## Baseline: the C11 mapping under `-imm`

`lkmm-c11.h` under `-imm`, on the 5571 corpus tests (2026-10-17, 30 s per
test, 27 minutes on 12 cores):

| result | tests |
|---|---|
| agree (states and execution counts) | 3498 |
| agree-racy (both report a data race) | 187 |
| skipped (174 LISA, 9 PPC, 31 SRCU, 6 other) | 220 |
| herd7 timeout (60 s) or rejects the test | 123 |
| disagree, all with a known cause (below) | 1543 |

| result | cause | tests |
|---|---|---|
| genmc-timeout | RCU emulation: `synchronize_rcu()` reads every reader's flag | 567 |
| count | RCU emulation: same states, more executions | 463 |
| extra | address dependency to a read (IMM's ppo ends in a write) | 249 |
| extra | strong-fence cumulativity (`smp_mb()`, `synchronize_rcu()`) | 69 |
| extra | release chains are A-cumulative in LKMM (`prop`) | 46 |
| missing | control dependency past the end of the `if` (IMM orders it) | 50 |
| missing | RCU emulation: the fences of `rcu_read_lock()`/`unlock()` | 26 |
| missing, race | `smp_rmb()`/`smp_wmb()` as acquire/release fences | 12 |
| race | plain accesses ordered by dependencies (C11 reports a race) | 41 |
| extra | `spin_unlock(); spin_lock()` is not ordered in C11 | 9 |
| extra | same-thread overwrite after a dependency (`overwrite & int`) | 8 |
| other | control dependency hides a race; mutex events | 3 |

`classify.py` assigns these from the primitives a test uses; every class was
checked by hand on examples. They are what a real LKMM model has to add to
IMM: `addr;[R]` in ppo, control dependencies that end at the `if`'s join,
`prop` (cumulativity, `prop & int`), LKMM's fences and locks, and RCU.

## Phase 1: `<lkmm.h>` and the kernel models

GenMC's `lli/runtime-include/c/lkmm.h` is now this C11 mapping (it was a
header of intrinsics nobody implemented); `lkmm-c11.h` only adds what
herd7's untyped litmus C needs. The rewrite reproduces the baseline: on the
corpus 5561 of 5571 results are identical, the other 10 are large RCU tests
that finished within 30 s in one run and not in the other (`compare-runs.py`
compares two `--json` runs). `spin_is_locked()` and `atomic_add_unless()`
translate now (kernel tests: 31 agree, 4 known gaps).

The kernel models of `tests/correct/classic` (wait queue, futex, refcount,
RCU list removal, SLAB_TYPESAFE_BY_RCU lookup) written with the kernel's
primitives are in `tests/correct/lkmm`, with their bug variants (`-lkmm`
suffix) in `tests/wrong`; they run under `-imm` with the mapping and pass,
with the same counts as the C11 versions except where the port differs (the
RCU list removal frees the entry after `synchronize_rcu()`, which the C11
version did not model; the lookup's lock-free CAS loops explore more).
The lock-less list (`llist`) fails under the mapping for a reason that is
LKMM's `addr` in `to-r` plus `wr-vis` and not a bug, so it waits in
`tests/cooking/lkmm` as an acceptance test for `-lkmm`.

The 82 v0.9 tests in `tests/cooking/lkmm` run under `-imm` with the
mapping: 51 give v0.9's LKMM count; the 31 others are RCU (more executions
or, for three `rcu+ar-link` tests, a safety violation: the reader-flag
emulation lacks LKMM's grace-period guarantee in those shapes), plain-access
races (C11 reports them), locks and `rmb` with no-return RMWs.

Two GenMC reports met on the way were false and are filed as separate
tasks (the ports sidestep them): a liveness violation for a retry loop
that contains a fenced CAS loop, and a same-thread data race under `-imm`
after a dependency-based revisit.

## Phase 2: the `-lkmm` model (marked accesses, fences, RMWs, locks)

`genmc -lkmm` checks programs written with `<lkmm.h>` against the kernel
model's regular part: `linux-kernel.cat`'s `ppo`, `prop`, `hb` and `pb`
without RCU (phase 3) and without the plain-access rules (phase 4). The
consistency checker `LKMMChecker` is generated by Kater from
`kat/lkmm-genmc.kat` in the Kater fork (GPL; see below); `(prop \ id) &
int`, which Kater cannot export, is computed by GenMC
(`genmc/Execution/LkmmRelations.cpp`). Under `-lkmm`, `<lkmm.h>` maps the
barriers to `__VERIFIER_lkmm_fence()` (an `LkmmFenceLabel` with the kind)
and the fully ordered RMWs carry the `Mb` attribute; everything else is the
C11 mapping. The models' `tests/correct/lkmm` category has the kernel-model
ports (phase 1, now also with `-lkmm` expected counts), v0.9's LKMM litmus
tests (54 of 82: the RCU and plain-access ones stay in `tests/cooking/lkmm`),
the lock-free `llist` (correct under `-lkmm` only: it relies on an address
dependency into a read) and the regression test of a bug found on the way.
`driver.sh` runs a correct test under `lkmm` iff it has an
`expected.lkmm.mo.in`, and a wrong test iff it has a `genmc.lkmm.<wb|mo>.in`.

Results (2026-10-19), all with herd7 7.58 as the oracle:

| suite | result |
|---|---|
| kernel `litmus-tests` (35 C tests) | 35 agree |
| corpus without RCU/SRCU (3167 tests, 30 s each) | 2886 agree, 70 agree-racy, 137 skipped, 12 herd7 timeout/error, 1 count; 62 classified disagreements (below) |
| `tests/correct/lkmm` (63 tests under their models) | pass |
| feature matrix (735 runs, below) | no deviation |
| v0.9's expected counts (52 tests) | identical; `R+fencembonceonces` 3 and `C-MPrelseq+o-r+rmwinc+a-o` 9 are herd7's (v0.9 had 4 and 10) |
| fast suite with the phase-2 binary (rc11, imm, lkmm) | passes |

| result | cause | tests |
|---|---|---|
| missing | control dependency past the end of the `if`: GenMC keeps every later event dependent on the branch | 28 |
| race, differ | plain accesses: races, plain-coherence and the fences of plain accesses are phase 4 | 33 |

`classify.py --model lkmm --allowlist` writes these into
`allowlist-lkmm.txt`, so a rerun fails only on new disagreements. The
control-dependency gap is permanent: GenMC's revisits keep events by their
positions, and a write after an `if` join changes position with the arm
taken, so the forward-closed `ctrl` must stay in the prefix relation (plan
3.6). Under `-lkmm`, GenMC therefore orders every event after a conditional
after the reads the condition depends on; it can miss an execution LKMM
allows when a compiler would hoist a store out of both arms.

Three bugs found and fixed on the way, each with a regression test:

- an RMW whose `genmc.attr` annotation carries only attribute bits (LKMM's
  fully ordered RMWs) hit an `UNREACHABLE` in `getFaiKinds()`
  (`tests/correct/lkmm/atomic-01` and three corpus tests crashed);
- GenMC makes the write of an RMW from its read, without dependencies of its
  own, so a kat dependency into an RMW must reach the write through `rmw`;
  without that, a lock taken under a condition between two critical sections
  of another thread was never explored (`tests/correct/lkmm/lock-ctrl-between`:
  4 executions, the `waitqueue` port lost 2 of its 6; `lb-dep-xchg` and the
  address and data variants pin the other dependencies);
- LKMM has no thread creation, and the kat put only `tc` and `tj` into its
  happens-before and prefix relation: a parent's read could read a value
  that depends on a thread it creates later, and a revisit prefix could hold
  a thread's events without its create, which failed an internal check (with
  `-schedule-policy=arbitrary`, half of the lkmm tests crashed). `thread-po`,
  IMM's release/acquire thread events, joins both
  (`tests/correct/lkmm/create-after-read`: 3 executions, as under RC11 and IMM).

Known gap: a `cmpxchg()`'s expected value orders nothing in LKMM, but GenMC
never builds the load-buffering cycle through it (the compare operand's
dependencies are recorded and read by no checker), so it misses that
outcome under `-lkmm` and `-imm` (`tests/cooking/lkmm/LB+casexp`: herd7 3
executions, GenMC 2).

Feature matrix (plan 5.6): `feature-matrix.py` runs every `tests/correct/lkmm`
test under `-nthreads=4`, `-schedule-policy=arbitrary` with three seeds,
`ltr` and `wfr`, `-disable-spin-assume`, `-check-liveness` and
`-mode=estimate`, and every wrong test that opts into `-lkmm` under the same
options with `-print-error-trace` and `-dump-error-graph`. Correct runs must
give the expected count, wrong runs the base run's error. Result
(2026-10-19): 735 runs, no deviation, with the liveness fix of
`fix-liveness-lock-block` (without it, `-check-liveness` crashes on
`2LU+LUL`, whose second thread ends holding the mutex; with it, that run
reports the liveness violation, which the matrix accepts for tests with
blocked executions).
The LKMM fence kinds print as `Fmb`, `Fwmb`, `Frmb`, `Faa`, ... in error
traces and as `F` with the kind in the dumped graphs.

How the kat maps onto GenMC's checker interface: Kater's reserved `ppo` is
the preserved program order of the revisit prefixes and must stay within
`po` (LKMM's own, with the cross-thread `smp_mb__after_unlock_lock()` rule,
is `lkppo`); the view given to `export coherence` decides the coherence
placements, so it is `po` and thread synchronisation only (LKMM's coherence
is sequential consistency per location); GenMC's reserved `hb` view, which
the initialisation checks, the helping CAS and the memory-error rules
consult, is porf (marked accesses publish by dependencies). The kat's
predicates cast through `EventLabel` (Kater instantiates them on labels of
any static type). `kater-export.sh --install lkmm` regenerates the checker.

The kernel-model ports under `-lkmm` versus `-imm`: `slab-rcu-lookup` has 9
executions instead of 11 (IMM lets the reader read the republished bucket
and then the stale key through the address dependency; LKMM's `to-r` orders
it), the others are equal.

## Kater and the generated checkers

GenMC's checkers (`genmc/genmc/Execution/Consistency/*Checker.{hpp,cpp}`)
are generated by Kater from kat files, which stay in the Kater repository:
Kater is GPL-3.0 (and an LKMM kat file, transcribed from the kernel's
GPL-2.0+ `linux-kernel.cat`, would be GPL too), while GenMC is Apache-2.0/MIT.

The public Kater 0.4.0 exports for an older GenMC interface. The branch
`genmc-0.19-export` of a Kater clone (`~/.local/src/kater`, three commits on
v0.4.0) exports for GenMC 0.19: view indices, init-label views, positional
relations, the fork's `retire_pos()` reordering (d8482e4), and
`kat/imm-genmc.kat` with the memory-error checks GenMC 0.19's `IMMChecker`
has (as in `rc11-genmc.kat`). `sc`, `tso`, `ra` and `rc11` are Kater's own.

    cd ~/.local/src/kater && git checkout genmc-0.19-export && cmake --build Release
    scripts/lkmm/kater-export.sh -o /tmp/checkers

Calibration result: the five regenerated checkers compile into GenMC 0.19,
and a build with all five passes the fast suite. They are not textually
identical to the checked-in ones: upstream generated those with a newer,
unreleased Kater (its automata are numbered and minimised differently), and
0.19's RA and RC11 kat files also have two views (`hbAcq`, `hbRel`) the
public ones lack. `compare-checkers.sh` compares the two builds program by
program under all five models: on every variant in `tests/correct` and
`tests/wrong` (3085 program/model pairs, 20 s each), verdicts and execution
counts are identical, except that the shipped RA and RC11 checkers time out
on `synthetic/barrier-det` (its default size; the suite uses `-DDIM=4`):
their extra `hbRel` view is slow there (8.6 s regenerated, over 120 s
shipped). The fast suite takes the same time with either (233 s shipped,
237 s regenerated, on an idle machine).
