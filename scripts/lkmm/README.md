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
| skipped (174 LISA, 9 PPC, 32 SRCU, 5 other) | 220 |
| herd7 timeout (60 s) or rejects the test | 123 |
| disagree, all with a known cause (below) | 1543 |

| result | cause | tests |
|---|---|---|
| genmc-timeout | RCU emulation: `synchronize_rcu()` reads every reader's flag | 567 |
| count | same, with the same states | 463 |
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
shipped).
