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
tests (54 of 82: the RCU ones came in phase 3, the plain-access ones in phase 4),
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

## Phase 3: RCU

Under `-lkmm`, `rcu_read_lock()`, `rcu_read_unlock()` and
`synchronize_rcu()` are events, as in herd7 (`LkmmFenceLabel` kinds
`rcu-lock`, `rcu-unlock`, `sync-rcu`); under the other models `<lkmm.h>`
keeps its reader-flag emulation. The kat puts the grace period into
`strong-fence` (`gp`, between accesses); the rest of RCU, `rcu-order`, counts
grace periods against critical sections and is no regular relation. GenMC
evaluates it directly: `genmc/Execution/LkmmModel.cpp` computes the model's
relations on the whole graph as bit matrices (with herd7's own `gp` and
`rcu-rscs`, the bell's innermost-first matching), `rcu-order` as the cat's
least fixpoint, and checks `irreflexive rb`. `LKMMRcuChecker`, a subclass of
the generated `LKMMChecker` (Kater's printer now makes its overrides
protected), runs it for graphs with a `synchronize_rcu()`, when the added
event can close an `rb` cycle. `-lkmm-reference` runs the evaluation on every
graph and stops where it and the generated checker disagree on coherence,
atomicity, `hb` or `pb` (plan 5.2); the RCU tests in `tests/correct/lkmm`
run with it.

A grace period orders a reader's accesses before a `kfree()` that follows
`synchronize_rcu()`, which GenMC's porf view does not see: the two
freed-memory rules (`last_accesses ; [FREE] <= hb`, `free ; [R | W] = 0`)
moved from the kat into `LKMMRcuChecker`, which also accepts an access
`(po | rf | tc | tj | rcu-fence)+`-before the free. A critical section still
open counts as closed at the end of its thread (the access is checked when
it is added, before its `rcu_read_unlock()`).

Invalid RCU usage, which the kernel's lockdep reports, is an error under
`-lkmm`: `rcu_read_unlock()` without a lock (herd7 flags
`unmatched-rcu-unlock`), a thread that ends in a critical section
(`unmatched-rcu-lock`), and `synchronize_rcu()` in a critical section (an
RCU self-deadlock: LKMM has no execution, `lkmm-diff.py` counts that as
`agree-deadlock`). A self-deadlock across threads (a critical section that
creates and joins a thread calling `synchronize_rcu()`) is not reported.

Bugs found on the way, each with a regression test that fails without its
fix:

- the driver does not check a read that takes the co-maximal write, or a
  fence: sound for orderings within ppo, but `rcu_read_unlock()` closes a
  critical section and so orders events already in the graph. It is now
  checked when added (`tests/correct/lkmm/rcu-gp2`: the forbidden outcome
  was reported as a safety violation);
- a revisit kept a read's later events but not the fences among them (no
  fence was in the revisit prefix): `synchronize_rcu()` and
  `rcu_read_unlock()` came back later in the middle of their thread,
  unchecked; and a revisited read that takes the co-maximal write skipped
  the check while `rcu-link` and `rcu-fence` order by `po`, not `ppo`. The
  kat's prefix relation has `[F] ; po` now (no access gains an ordering:
  whatever ppo leads from an event into a fence already orders it before
  every event after the fence), and `LKMMRcuChecker` refuses the
  co-maximal shortcut for a read that is not the last of its thread
  (`tests/correct/lkmm/rcu-gp-revisit`, the corpus's `C-RW-G+RW-R1`: five
  corpus tests allowed a forbidden state, with either fix alone too);
- the freed-memory check needs the open critical sections closed: without
  it, `rcu-list-del` reported a use of freed memory under
  `-schedule-policy=arbitrary` (`tests/correct/lkmm/rcu-list-del`, seed 1);
- strong fences ordered only accesses, as the cat's `[M]` has it, but
  GenMC's thread creation and join synchronise like a release and an
  acquire: `WRITE_ONCE(x, 1); smp_mb(); pthread_create()` did not order the
  child's writes after `x` for a reader with `smp_rmb()` (allowed under
  `-lkmm` since phase 2), and with `synchronize_rcu()` the generated checker
  and the model disagreed (found by the review; `tests/correct/lkmm/fence-thread`:
  creation and join, `smp_mb()` and `synchronize_rcu()`; under RC11 and IMM
  the outcome is allowed, as `y` is written without a release). The kat's
  `mb`, the model and `prop-int` let a strong fence reach thread events.

Tests: v0.9's RCU tests (16, v0.9's counts; `rcu+wb-ext` is the wrong
test `tests/wrong/safety/rcu+wb-ext-lkmm`, herd7 allows its outcome), the
kernel's `RCU+sync+free` with `call_rcu()` emulated
(`tests/correct/lkmm/rcu-sync-free`; without the grace period,
`tests/wrong/memory/rcu-sync-free-no-gp-lkmm`), publication with
`rcu_assign_pointer()` (`rcu-publish`; with `WRITE_ONCE()`,
`tests/wrong/safety/rcu-publish-write-once-lkmm`), liburcu's urcu-mb on
the kernel's primitives (`urcu-mb`, 34 executions as in C11; one phase
flip, `tests/wrong/memory/urcu-single-flip-lkmm`), the misuses
(`tests/wrong/locking/rcu-invalid-lkmm`), message passing through thread
creation and join after a strong fence (`fence-thread`), and the kernel ports, whose RCU is
now the model's: `rcu-list-del` has 2 executions (3 with the emulation),
`slab-rcu-lookup` 9. A wrong test named `*-lkmm` runs under another model
only with that model's `genmc.<model>.*.in`.

Results (2026-10-19), herd7 7.58:

| suite | result |
|---|---|
| corpus with `rcu_read_lock`/`synchronize_rcu`, no SRCU (1544 files) | 1375 agree, 1 agree-racy, 8 agree-deadlock, 47 skipped (LISA), 109 herd7 timeout, 2 herd7 error, 2 race (plain accesses, phase 4) |
| corpus without RCU (3167 files) | as in phase 2 |

The slowest RCU tests are 7-thread grace-period tests with 16384
executions (herd7: 58 s): alone, without `-print-exec-graphs`, they take
about 5.5 s (20.8 s at first). The RCU check is evaluated only where an
`rb` cycle can close, skips the generated check for a fence at the end of
its thread, computes `rcu-link`, `rcu-order` and `rb` only for the rows of
RCU's events, leaves out the plain accesses that cannot reach `rb` (outside
`-lkmm-reference`), and remembers `prop-int` within one check. The
12-thread grace-period tests take about 2 s (herd7: 26 s). In the corpus
run (8 jobs, graphs printed) none times out at 30 s. The RCU tests of
`tests/correct/lkmm` run with and without `-lkmm-reference`.

## Phase 4: plain accesses and data races

GenMC's plain (non-atomic) accesses are events of the graph, but a plain
read takes the co-maximal write and is never revisited: under C11's
DRF guarantee, race detection against the hb view makes that exact. LKMM
has no such guarantee: plain-coherence constrains what a plain read may
see by fences and `xbstar` (`(hb | pb | rb)*`), which are not porf. Under
`-lkmm`, plain loads and stores therefore go through the driver's marked
path (`handleLoad`/`handleStore`, with the `NotAtomic` ordering): a plain
read gets every coherent write, a plain write every coherent place in `co`.

The plain-access part of the cat intersects relations, which Kater cannot
export, so `LkmmModel.cpp` evaluates it: `vis`, the four boundaries
(`w-pre-bounded`, ...), `ww-vis`, `wr-vis`, `rw-xbstar` and `pre-race`,
with `rcu-fence` in `fence` and `strong-fence` and `rb` in `xbstar`.

- **plain-coherence** is an axiom: `LKMMRcuChecker` evaluates it when the
  graph has a potential race and the added event can break it (as for
  `rb`: not an event at the end of its thread with no successor). For a
  plain access, the generated check is skipped: it is no event of `hb`,
  `pb` or `rb`, and its `rf`/`co` edges are no steps of them.
- **data races** are a flag in herd7 and an error in GenMC (`Non-atomic
  race`, with the race kind): `lkmm::findRace()` runs on every complete
  execution. A pair can be ordered by events added after both (`pb`,
  `rb`, the post-bounds of the other thread), so LKMM's races cannot be
  checked when an access is added, as the C11 models' are, nor on the
  prefix where an assertion fails; the kat's C11 race rule is gone.
  `-disable-race-detection` turns the check off and leaves the racy
  executions, which plain-coherence still shapes.

GenMC's additions, as for the marked part: thread creation and join count
as a release write read by an acquire read (the thread events are releases
and acquires to `po-rel`, `acq-po`, the fences and `cumul-fence`; `tc`, `tj`
are `rfe` to `vis`), and the accesses they order, `(po | tc | tj)+`, are no
potential races (GenMC's coherence view orders them already). Without the
latter a parent's plain read after `pthread_join()` raced with the child's
plain write in every litmus translation, and the full evaluation ran on
every check.

The revisit prefix (the kat's `ppo`) no longer orders a plain access before
the events a fence or a lock handover puts after it: `lkhb` has no plain
events, so LKMM allows a load-buffering cycle through a plain read that
precedes a release (in a racy execution, or one plain-coherence forbids).
With it in the prefix, GenMC missed one racy execution in 23 corpus tests
(`auto/C-LB-Lrw+R-Oc` and its relatives; `tests/correct/lkmm/plain-lb-racy`,
3 executions with `-disable-race-detection`, 2 before). A write whose
location has no read computes no revisit prefix (under IMM and LKMM, whose
checkers do not take a read's prefix from its write's): the litmus
translations' register stores made that half of the time.

`lkmm-diff.py` runs a racy test (by either tool) a second time with
`-disable-race-detection` under `-lkmm` and compares its states too: an
`agree-racy` result now means the same race verdict and the same states.
herd7's `mixed-accesses` flag (3 corpus tests) is not reported: it needs
`barrier()`, which is no event in GenMC.

Tests: v0.9's plain-access tests, with herd7's counts where v0.9's
differed (`plain-mp+rcuderef`, `plain-S-rcuderef`, `plain-S-rcunoderef2`:
2, not 3; `plain-strong-vis`: 4, not 5): the race-free ones in
`tests/correct/lkmm` (11; `plain-nonrace4` was a wrong test, but neither
tool finds a race), the racy ones as `tests/wrong/racy/*-lkmm` (13);
`plain-nonrace1`, `plain-lb2` and `plain-wmb-race2` stay in
`tests/cooking/lkmm` (control dependency past the `if`), with herd7's
counts. New: `plain-coherence-lb` (a plain read before `smp_mb()` cannot
see a write after an acquire of the flag it wrote: `wr-incoh`),
`plain-coherence-ww` (two plain writes ordered by release/acquire keep that
order in `co`: `ww-incoh`), `plain-thread-sync` (creation and join, also
through a chain and a join followed by a release, order plain accesses),
`plain-lb-racy`. Each fails without its rule. The kernel port's
`refcount-dec-no-acquire-lkmm` stays out of `-lkmm`: its assertion fails
before an execution completes (the stale read is LKMM's `wr-race`).

Results (2026-10-19), herd7 7.58:

| suite | result |
|---|---|
| whole corpus, no SRCU (4711 files, 30 s each, 10 jobs) | 4262 agree, 97 agree-racy (same race verdict and states), 8 agree-deadlock, 184 skipped, 117 herd7 timeout, 6 herd7 error, 1 count; 34 classified disagreements: 27 missing and 3 race (control dependency past the `if`), 4 missing (herd7's symbolic thin-air values); 2 GenMC timeouts under load (the 7-thread grace-period tests, 6.8 s alone) |
| kernel `litmus-tests` (35) | 35 agree |
| `tests/correct/lkmm`, `tests/wrong/racy` (rc11, imm, lkmm) | pass |
| feature matrix | 1594 runs, no deviation |

The 23 `auto/C-LB-Lrw*` tests (races through a plain read before a
release) and the plain tests that were allow-listed as phase 4 agree now,
racy states included.

## Phase 5: SRCU

herd7 models SRCU with memory events of the `srcu_struct`:
`srcu_read_lock(s)` reads it and returns the value read,
`srcu_read_unlock(s, idx)` writes `idx` back, and `synchronize_srcu(s)` is
a fence whose location is `s`. GenMC does the same under `-lkmm`, with
three intrinsics (`__VERIFIER_srcu_read_lock`, `_unlock`,
`__VERIFIER_synchronize_srcu`), two new label kinds (`SrcuLockRead`,
`SrcuUnlockWrite`, plain reads and writes for the rest of GenMC) and an
`LkmmFenceLabel` of kind `SyncSrcu` that carries the address. The lock and
unlock events are marked accesses, so they take part in `rf` and `co` as
herd7's do: two empty critical sections of one `srcu_struct` already have
4 executions (each lock may read the other's unlock), and GenMC explores
the same ones.

- **srcu-rscs**: an unlock matches the locks of its `srcu_struct` whose
  value reaches its own by `data ; ([~Srcu-unlock] ; rf ; data)*`, so
  `srcu_down_read()` and `srcu_up_read()` may be in different threads,
  with the index passed through memory. `lkmm::srcuRscs()` follows GenMC's
  data dependencies back from each unlock. GenMC's thread creation and
  join pass a value as a write read by a read would (herd7 has no
  threads): a thread's argument depends on its start event, the create on
  the argument passed, a joined value on the join, the thread's end on the
  value it returns.
- **rcu-order** gains `srcu-gp` and the four SRCU clauses, each `& loc`
  (a grace period orders only the critical sections of its
  `srcu_struct`); `gp` and `strong-fence` gain `Sync-srcu`, `mb` gains
  `smp_mb__after_srcu_read_unlock()`, and `carry-dep` no longer passes
  through an unlock (a lock that reads an index back depends on nothing).
  The kat (Kater 84bc51e) has the regular part, `LkmmModel.cpp` the rest.
- An unlock closes a critical section, so the driver checks the graph
  when one is added, as for `rcu_read_unlock()` (a write is otherwise
  placed co-maximal without a check). A `synchronize_srcu()` inside a
  section of its own `srcu_struct` (same thread) waits for its own reader:
  every execution is inconsistent (herd7: none, no flag), and GenMC reports
  invalid RCU usage there, as for `synchronize_rcu()`.
- An access of freed memory is checked when added, against the frees in
  the graph, with a critical section still open taken as closed at the
  end of its thread. A section that `srcu_up_read()` closes in another
  thread can end earlier (before accesses after the handoff), so an
  execution with such a section is checked again when complete, with
  every section matched.
- **The bell's flags** are errors (`Invalid RCU usage`), checked on
  complete executions (the matching may cross threads): an unmatched lock
  or unlock, a lock with two unlocks (`multiple-srcu-matches`), an unlock
  of another value than its lock read (`srcu-bad-value-match`), and
  `synchronize_srcu()` in an RCU critical section (`invalid-sleep`, at the
  event). `lkmm-diff.py` reports `agree-flag` when herd7 flags SRCU misuse
  and GenMC reports it, `flag` when only herd7 does.

`<lkmm.h>` provides `struct srcu_struct` (an `int`), `DEFINE_SRCU`,
`init_srcu_struct()`, the lock, unlock and grace-period calls with their
`_expedited`/`down`/`up` forms, `srcu_dereference()` and
`smp_mb__after_srcu_read_unlock()`, under `-lkmm` only. `litmus2c.py
--srcu` (which `lkmm-diff.py` passes under `-lkmm`) translates the SRCU
tests. The rest of the plan's phase 5 was in place already:
`smp_cond_load_{relaxed,acquire}()` (an await), `atomic_add_unless()`,
`spin_is_locked()`; herd7's model has no `smp_cond_load` (its tests stay
skipped).

Tests: `tests/correct/lkmm/srcu-gp` (a grace period waits for a reader of
its `srcu_struct`), `srcu-gp-other` (not for one of another),
`srcu-down-up` (a section opened and closed in different threads, the
index through memory, with `smp_cond_load_acquire()`), `srcu-mb-after`
and `srcu-mb-after-none`, `srcu-sync-free` (call_srcu() emulated, as
`rcu-sync-free`), all with herd7's counts of litmus twins;
`srcu-handoff-free` (a section closed in another thread, with the
dereference before the handoff) and `srcu-thread-handoff` (the index
through `pthread_create()` and `pthread_join()`);
`tests/wrong/locking/srcu-invalid-lkmm` (six misuses that herd7 flags,
and a grace period in its own section) and
`tests/wrong/memory/srcu-sync-free-bug-lkmm` (no grace period, one of
another `srcu_struct`, a dereference after the handoff). A review found
the last three gaps (a missed use after free, which depended on the
scheduling; a false unmatched lock for an index passed to or returned
from a thread; no report for a grace period in its own section): each
test fails without its fix.

Results (2026-10-19), herd7 7.58:

| suite | result |
|---|---|
| corpus tests with SRCU (52 files) | 51 agree (states and execution counts, also with `-lkmm-reference`; `C-srcu-nest-4` as `agree-flag`), 1 skipped (LISA) |
| whole corpus (5571 files, 30 s each, 10 jobs) | 5006 agree, 192 agree-racy, 8 agree-deadlock, 1 agree-flag, 185 skipped, 117 herd7 timeout, 6 herd7 error, 1 count; 55 classified disagreements: 48 missing and 3 race (control dependency past the `if`; 20 of them, `auto/C-LB-LRW+R-Dd*`, were not in phase 4's 4711-file run and are classified now), 4 missing (herd7's symbolic thin-air values); no GenMC timeout |
| kernel `litmus-tests` (35) | 35 agree (with `--strict-counts`) |
| `tests/correct/lkmm`, `tests/wrong` (rc11, imm, lkmm) | pass |
| feature matrix | 1805 runs, no deviation |

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
