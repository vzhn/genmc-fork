# LKMM tooling

Tools for adding the Linux-kernel memory model (LKMM) to GenMC and for
checking it against herd7, the reference implementation of LKMM.

| Tool | What it does |
|---|---|
| `run-herd.sh` | runs herd7 with LKMM on C litmus tests, one `.out` per test |
| `litmus2c.py` | translates a C litmus test into a GenMC program |
| `lkmm-diff.py` | runs GenMC on the translations and compares with herd7 |
| `lkmm-c11.h` | kernel primitives mapped onto C11 atomics (an approximation, for `-imm`) |

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
anything else that disagrees makes the exit status 1.
