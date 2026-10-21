# Changelog

Notable changes to GenMC will be documented in this file. This
repository is only updated whenever a new version of GenMC is
released.


## [Unreleased]

- Merge of MIXER [POPL'25] into mainline GenMC

### Fixed

- A logical negation in an annotation expression always evaluated to 0
  (`SExprEvaluator::visitNotExpr()` negated the operand's pointer, #64).
  Loop jump threading never inverted a loop whose header leaves it on the
  true side of its branch (`for (;;) { if (r) break; r = ...; }`), so the
  spin-assume pass did not recognize such a loop and GenMC did not
  terminate on it.
- `-lkmm`: a run whose thread reached, at a hole, an instruction whose kept
  event lies behind kept events of a branch the thread does not take went
  on until the thread's end instead of being dropped at once (#109). It added a
  second event for the instruction, placed it in coherence against the
  kept events, could report a spurious write-write race (against a hole),
  and queued revisits whose graphs contradict coherence; `-lkmm-reference`
  stopped on them with "The LKMM checker and the model disagree".

### Added

- The expression evaluator's arithmetic is in `genmc/Support/SExprOps.hpp`,
  as functions of the operands' values and width; `scripts/prove-sexpr-ops.sh`
  proves them equal to LLVM's integer semantics for all inputs with Alive2.
- Debugging options for the transformation passes: `-output-llvm-passes=<dir>`
  writes the module before and after each pass that changes it, in a form
  Alive2 can check (and, for spin-assume and the load annotations, with
  what they claim), and `-verify-pass-analyses` checks the dominator trees
  passes declare preserved. `scripts/check-passes.sh` runs both over test
  programs and checks every pass with Alive2's `alive-tv`.
- `-exhaustive`, a reference exploration for small programs: every
  consistent backward revisit is explored, graphs are deduplicated by a
  canonical hash (threads named by their creation path), and every
  reduction is off. `scripts/oracle-diff.sh` compares the default
  exploration with it over the test suite (ctest target `oracle-diff`),
  and `scripts/fuzz-c11.py` over random C11 programs, under arbitrary
  schedules and without resume points too.

### Fixed

- A weak CAS lost executions in three ways: its spurious-failure flag
  survived into the forward revisits that followed (#116); the failure
  revisit of a CAS that a backward revisit made succeed cut the write it
  reads from (an internal failure under rc11 and imm, #117); and its
  spurious failure reading a write that a settled RMW reads was never
  explored (#118).
- `-imm` segfaulted when a revisit kept a `fetch_add`'s write part without
  its read part (#120), and left that write stale (an inconsistent
  execution) or relocated it when the read was executed anew (#122).


## [0.19.0-vzhn.13] - 2026.10.21

Fork release on top of 0.19.0 (https://github.com/vzhn/genmc-fork).

### Fixed

- `-lkmm` lost executions when a backward revisit kept a thread's write
  to a location while deleting a read of it that comes before (#100,
  #105): the read, re-executed into the hole, cannot read the po-later
  write, yet it counted as not added maximally, and every later revisit
  that deleted it was rejected. A two-worker program handing pooled
  buffers from one worker to the other explored 692 complete executions
  instead of rc11's 11,786. This affected every `-lkmm` release.
- `-lkmm` reported "Attempt to access freed memory" for an access that a
  thread re-added, after a revisit, before its own kept free (#96, #97).
- `-lkmm` warned "Unordered writes" about writes that a lock or a
  release/acquire pair orders, naming the initial event (#98, #99). The
  model's own rule alone decides now.
- Every run left the compiled module's temp file `/tmp/__genmc.ll.XXXXXX`
  behind (#102).

### Performance

- Finished threads are no longer replayed at the start of each execution,
  and forward revisits resume from saved interpreter states
  (`-resume-points=<n>`, default 256) instead of re-executing the graph
  (#104).
- `FunctionInlinerPass` inlines callees first, through their uses, instead
  of rescanning the module for each function (#103): the transformation
  phase of a nine-file program went from 5.5 s to 2.3 s.

### Added

- Debugging options to check the transformation passes with Alive2 (#101):
  `-output-llvm-passes=<dir>` writes each pass's module before and after
  it in a form `alive-tv` can check, and `-verify-pass-analyses` checks
  the analyses the passes declare preserved.

### Tests

- `tests/correct/lkmm/hole-read-kept-write`, `kept-free-access`,
  `ww-lock-ordered`, `ww-mp-ordered` and `ww-unordered` (the last three
  checked by `scripts/check-lkmm-ww-warning.sh`, ctest `lkmm-ww-warning`).


## [0.19.0-vzhn.12] - 2026.10.20

Fork release on top of 0.19.0 (https://github.com/vzhn/genmc-fork).

### Fixed

- `-lkmm` lost executions, and could miss bugs, when a backward revisit
  deletes a read of a location that the revisiting thread writes again
  before a later, kept read of it, e.g. after `if (x) x = 0;` (#88, #94).
  The revisit was rejected as non-maximal, so a program whose assertion
  fails in every model passed, and an await-based client explored no
  complete execution at all. This affected 0.19.0-vzhn.9 to vzhn.11.

### Performance

- `-lkmm`: the whole-graph check after events added before kept ones runs
  once per run instead of after each such event (#94); a run that fails it
  is repeated with the check after each event ("Runs repeated with eager
  consistency checks"). An await-based client went from 20 minutes to 53s.
- Cheaper per-event checks in the consistency checkers, symmetric joins and
  annotation scans, which made lock-heavy programs with many threads
  superquadratic (#89).

### Tests

- `tests/correct/lkmm/PropGpPulp`: a grace period ending at a
  `synchronize_rcu()` and a lock handover from it, GenMC's narrower `gp`
  (#93).


## [0.19.0-vzhn.11] - 2026.10.20

Fork release on top of 0.19.0 (https://github.com/vzhn/genmc-fork).

### Changed

- When in-place revisiting (IPR) turns unordered writes into an error,
  GenMC prints a tip and starts over without IPR instead of stopping (#86).
  The program may have a bug that the error hid.

### Fixed

- Symmetry reduction missed bugs that a join can observe (#86): joining a
  symmetric thread before its symmetric predecessors (e.g., a race with a
  predecessor that is still running) or using the return value of a joined
  symmetric thread. Such joins are now an error ("Execution not well-formed
  for symmetry reduction"); with inferred symmetry, GenMC starts over
  without symmetry reduction. This also affected 0.19.0-vzhn.10, where
  more programs get symmetry reduction, and earlier versions.
- Spinloop detection treated an iteration whose only effect is creating a
  thread or freeing memory as side-effect free (#84): e.g.
  `while (!flag) pthread_create(...)` was cut after the first spawn and
  bugs needing a second thread were missed.
- `scripts/svcomp`: a run that ended unsuccessfully without an error line
  (e.g., unordered writes treated as an error) no longer counts as a safe
  verdict (#86).


## [0.19.0-vzhn.10] - 2026.10.20

Fork release on top of 0.19.0 (https://github.com/vzhn/genmc-fork).

### Changed

- Symmetry reduction applies to more programs (#76): to workers spawned in
  a loop (accesses to a local variable whose address never leaves its
  function no longer break symmetry, e.g. `pthread_t t[N]`) and to threads
  that ignore their argument. Execution counts drop accordingly (25 tests
  of the suite). When every symmetry was inferred and SR meets unordered
  writes, GenMC prints a tip and starts over without SR instead of failing.
- Spin loops that wait under a mutex are detected (#82): taking and
  releasing a mutex in an iteration is no longer a side effect, so such
  loops terminate. An iteration that released a mutex counts as observable
  for liveness.
- `-lkmm`: the dependencies GenMC keeps for an event after a branch, for
  what the branch's code could order on another path, are only in GenMC's
  prefix, not in LKMM's ordering (#83). This removes lost executions where
  they made an execution herd7 allows inconsistent, and a write may now
  revisit a read that is in its prefix only through them.

### Fixed

- `-lkmm`: an execution could be explored twice when a revisit kept a
  write placed non-maximally in coherence (#75).
- `-lkmm`: an event added before events kept further on in its thread
  could walk an inverted coherence range (an abort depending on the heap),
  and with `-nthreads` the write-write race warning crashed on every run of
  such a program (#81).
- `LoopJumpThreadingPass` left a stale dominator tree, and the spin-assume
  pass then built a broken module ("Instruction does not dominate all
  uses") (#82).
- `scripts/svcomp` builds against the new `pthread.h` condition variables
  and finds `main(argc, argv)` in included headers (#80).

### Added

- `scripts/lkmm/litmus2c.py --local-regs` (#79), and `matrix-skip` files
  for `scripts/lkmm/feature-matrix.py` (#81).


## [0.19.0-vzhn.9] - 2026.10.20

Fork release on top of 0.19.0 (https://github.com/vzhn/genmc-fork).

### Added

- `-lkmm`: a control dependency ends at the join of its branch, as in the
  kernel model (#73): GenMC finds the executions (and bugs) where a write
  after an `if` is reordered before the read the condition tested. A
  revisit may now keep events after a branch that the re-run thread takes
  with another length; GenMC moves them to where their instruction runs.
  45 of the 51 corpus tests that disagreed with herd7 for this agree now;
  the manual lists the remaining differences.
- `-check-replay-keys` (debugging, #73): when GenMC re-runs a thread and
  reuses an event of the graph, check that the same instance of the same
  instruction produced it (an internal error otherwise). Always on under
  `-lkmm`.
- `--blind-rmw-reduction` (RC11, #69, #72): explores one coherence order of
  identical relaxed RMWs whose results are unused, instead of all of them;
  executions that could observe their order are reported as not
  well-formed.
- pthread condition variables, modelled on `__VERIFIER_await()` (#67).
- `tests/lib/go.h`, a model of Go's runtime, and 59 GoBench kernels as
  tests (#62).
- `scripts/lkmm/fuzz-ctrl.py`: random litmus tests with branches (#73).

### Fixed

- `-lkmm`: a crash on an indirect call whose target depends on a read, and
  on a failed `cmpxchg()` that a revisit makes succeed (#73).
- `-check-liveness` no longer reports a wait in an execution that a failed
  `__VERIFIER_assume()` cut off (#63).
- Await, spin-loop and assume conditions are evaluated at their
  expressions' widths, with their casts (#65): narrow arithmetic gave false
  liveness violations.
- Under dependency-tracking models (IMM, LKMM), two allocations of a thread
  could get overlapping addresses after a revisit (#66): false
  non-allocated-memory and double-free reports.
- From the SV-COMP pilot (#67): the message of a failed assertion is
  printed again; `abort()` is reported as an error instead of killing
  GenMC; a call with too few arguments is reported instead of crashing;
  `malloc(0)` and `calloc(0, n)` return a unique pointer that can be freed.


## [0.19.0-vzhn.8] - 2026.10.19

Fork release on top of 0.19.0 (https://github.com/vzhn/genmc-fork).

### Added

- `-lkmm`: SRCU, as the kernel model has it (#58): `srcu_read_lock()`,
  `srcu_read_unlock()`, `srcu_down_read()`/`srcu_up_read()` (also across
  threads), `synchronize_srcu()`, `smp_mb__after_srcu_read_unlock()` in
  `<lkmm.h>`. Misuse (unmatched or multiply matched locks and unlocks, a
  bad index, a grace period in an RCU section or in a section of its own
  `srcu_struct`) is reported as invalid RCU usage. All of the litmus
  corpus's SRCU tests agree with herd7, execution counts included.
- The manual describes `-lkmm` (#59): `<lkmm.h>`'s primitives, what GenMC
  checks, and how it relates to herd7, with the known differences.

### Fixed

- The results of atomic add, sub and nand are wrapped to the access width
  (#55): `fetch_add(&int_var, -1)` on 1 stored 2^32, which gave false
  safety and liveness violations.


## [0.19.0-vzhn.7] - 2026.10.19

Fork release on top of 0.19.0 (https://github.com/vzhn/genmc-fork).

### Added

- `-lkmm`: the kernel model's plain-access rules (#52). Plain accesses are
  explored like marked ones (a plain read may read any write the model
  allows), plain-coherence is enforced, and LKMM's data races are reported
  as `Non-atomic race` errors, on complete executions
  (`-disable-race-detection` turns that off). Thread creation and join
  synchronise like release/acquire. Not reported: herd7's
  `mixed-accesses` warning. Not yet: SRCU.
- `scripts/svcomp/`: run GenMC on SV-COMP's concurrency tasks (#50).
- `scripts/lkmm/lkmm-diff.py` compares a racy test's states too.
- Tests: concurrency bugs from Rust libraries (tokio, crossbeam,
  parking_lot, std) (#51); v0.9's LKMM plain-access tests with herd7's
  counts.

### Changed

- `-lkmm`: a faster RCU check; the slowest corpus RCU tests take about
  5.5 s instead of 9-13 s (#43).
- Under IMM and LKMM, a write whose location has no read computes no
  revisit prefix.


## [0.19.0-vzhn.6] - 2026.10.19

Fork release on top of 0.19.0 (https://github.com/vzhn/genmc-fork).

### Added

- `-lkmm`: the Linux-kernel memory model, for programs written with
  `<lkmm.h>` (#38, #41). The consistency checker is generated by Kater from
  a kat transcription of the kernel's `linux-kernel.cat` (kept in a Kater
  fork: GPL); `(prop \ id) & int` and RCU's `rcu-order` are computed by
  GenMC. Supported: `READ_ONCE()`/`WRITE_ONCE()`, acquire/release, the
  kernel's barriers (`smp_mb()`, `smp_rmb()`, `smp_wmb()`,
  `smp_mb__{before,after}_atomic()`, `smp_mb__after_spinlock()`,
  `smp_mb__after_unlock_lock()`), value-returning and void atomics, spinlocks,
  and RCU (`rcu_read_lock()`, `rcu_read_unlock()`, `synchronize_rcu()`,
  with `kfree()` after a grace period). Invalid RCU usage (an unmatched
  unlock, `synchronize_rcu()` in a read-side critical section, a thread
  ending in one) is an error. Not yet: plain-access races (LKMM's rules),
  SRCU. Under the other models `<lkmm.h>` maps the primitives onto C11.
- `-lkmm-reference`: check every graph with a direct evaluation of the
  kernel model too, and stop where it and the generated checker disagree.
- `scripts/lkmm/`: differential testing against herd7 on the kernel's and
  Paul McKenney's litmus corpora; results and known gaps in
  `scripts/lkmm/README.md`.
- Tests: lock-free data structures and textbook concurrency bugs (#34),
  lock bugs from libvsync, DPDK, seL4 and Linux (#35), Linux kernel barrier
  bugs (#39), glibc and DPDK primitives (#40), and the `lkmm` category.

### Fixes

- `-check-liveness` crashed (an internal check) on a thread blocked in a
  mutex lock or a condition-variable wait; such threads are now reported as
  liveness violations when they can never proceed (#36).
- Spin-assume: a lock or fetch-and-increment loop rewritten into a
  zero-net-effect spin end blocked an iteration that leaves the loop, so
  assertions after the loop were missed under every model (#37).

### Changed

- `expected-error.in` of a wrong test may list several acceptable errors,
  one per line; the fast test driver allows 600 seconds per test (#34).


## [0.19.0-vzhn.5] - 2026.10.19

Fork release on top of 0.19.0 (https://github.com/vzhn/genmc-fork).

### Fixes

- Code condenser: a block was merged into a bisimilar one even when values of
  the merged ranges were used elsewhere or the surviving range used values
  the redirected paths never define (e.g. `first = load(x)` before a loop
  reloading `x` was redefined by every iteration), so assertion violations
  were missed with default flags (#29).
- Load annotation / IPR: instructions the annotator cannot model (floating-
  point compares and conversions, ...) were replaced by `true`, which turned
  the annotation of e.g. `while (load(x) == 0.0f);` into `assume(false)`; the
  loop never exited and later assertions were missed. Such annotations are
  now dropped (#30).
- Re-executing a thread's kill event (`-unroll` bound) during an error replay
  that targets the kill, e.g. for the unfreed-memory warning, or with race
  detection disabled, failed an internal check (#31).
- Thread begin labels whose create event was cut by a restriction of the
  graph: under IMM they were kept (hole labels), a non-trailing thread could
  be erased (shifting thread ids), and threads created by other threads could
  not be removed at all; all three ended in internal failures or a segfault
  with symmetry reduction. Such begins are now removed or kept terminated and
  revived when their create is executed again (#32).

### Added

- Regression tests for the above: `wrong/safety/condenser-reused-value`,
  `wrong/safety/annot-unmodeled-inst`, `correct/infr/kill-error-replay`,
  `correct/infr/kill-replay-norace`,
  `correct/data-structures/mpmc-queue-unroll-kill`,
  `correct/infr/nested-create-{revisit,erase,symm}`,
  `correct/infr/pool-protocol-arbitrary`.


## [0.19.0-vzhn.4] - 2026.10.18

Fork release on top of 0.19.0 (https://github.com/vzhn/genmc-fork).

### Fixes

- Spin-assume: loops whose header carries state from one iteration to the
  next were pruned as if each failed iteration repeated the previous one. The
  C11 CAS retry loop (`old = load(x); while (!cas(&x, &old, ...))`, where a
  failed CAS writes the value it read into `old`), loops that refresh a value
  with a load, and CAS success flags set before the loop were affected: GenMC
  could miss safety violations reachable only through a retry, and
  `-check-liveness` reported terminating loops as non-terminating spinloops.
  Such loops now reload the value at the top of the loop when that is sound,
  which keeps their exploration cost; otherwise only iterations that leave
  the loop state unchanged are pruned (#26)

### Added

- Tests: classic concurrency problems, correct and with known bugs
  (`correct/classic` and variants in `wrong/{safety,liveness,racy}`), with
  shared `tests/lib/{lock,sem,cond}.h`
- A wrong test can name the error it must find (`expected-error.in`)
- Tests for the spin-assume fix (`spin-cas-writeback*`, `spin-phi-*`,
  `spin-load-phi-*`, `spin-weak-cas-phi`, `spin-cas-order-race`)


## [0.19.0-vzhn.3] - 2026.10.16

Fork release on top of 0.19.0 (https://github.com/vzhn/genmc-fork).

### Fixes

- `-nthreads`: a worker could sleep while a task was queued (lost wakeup),
  workers kept taking tasks after another worker found an error and halted the
  pool, and the main thread busy-waited for the workers to finish
- `-nthreads`: an error replay left pending by one task leaked into the next
  task of the same worker, which could crash or garble the error report
- `-check-liveness`: a liveness violation whose blocked read was non-atomic was
  lost (the replay dropped its target) or aborted the report; such violations
  are now reported, with a warning if source information may be missing

### Added

- Tests: a model of the thread-pool protocol (`correct/infr/pool-protocol`,
  `wrong/safety/pool-*`), `wrong/safety/parallel-error-report`,
  `wrong/liveness/{spin-na-stuck,await-stuck-join}`, and
  `check-liveness-report.sh` (ctest `liveness-report`)


## [0.19.0-vzhn.2] - 2026.10.16

Fork release on top of 0.19.0 (https://github.com/vzhn/genmc-fork).

### Fixes

- `__VERIFIER_await()` inside a loop (e.g., a retry loop) was reported as a
  non-terminating spinloop; awaits now have their own markers (`AWAIT_START`,
  `BLOCK[await]`) and leave the loop's state alone. A stuck await is reported
  as a non-terminating wait


## [0.19.0-vzhn.1] - 2026.10.16

Fork release on top of 0.19.0 (https://github.com/vzhn/genmc-fork).

### Added

- `__VERIFIER_await(cond)`: a wait that `-check-liveness` reports when it can
  never finish (`__VERIFIER_assume()` keeps pruning silently); without
  `-check-liveness`, a tip points at the flag when executions ended in a wait
- `calloc()`: a zero-initialized allocation is a single event; the block counts
  as initialized by it, so zeroing no longer adds one store per byte to the graph
- Test scripts: `run-parallel.sh` (the fast suite with `-nthreads`, ctest
  `run-parallel`), `tsan-parallel.sh` (ctest `tsan-parallel` in ThreadSanitizer
  builds), `check-liveness-tip.sh` (ctest `liveness-tip`)
- Mechanized soundness proof of TruSt (Algorithm 1) in Rocq (`proofs/`)

### Fixes

- `-nthreads` > 1: function pointers stored in memory pointed into another
  worker's copy of the module (unknown external function, or a crash)
- `-nthreads` > 1 under IMM: graph copies shared `DepView` storage between
  workers (heap corruption); a race on the thread pool's worker list
- Memory intrinsics are promoted by byte offset, and `memset` honors its length
  and byte value
- Load annotation no longer enumerates every CFG path (exponential on inlined code)
- The interpreter supports `freeze`

### Performance

- Annotated loads concretize only the registers the annotation mentions
- Interpreter frame values in a `DenseMap`; no string copies per instruction
- The use-after-retire check asks the cheap retire lookup before the
  hazard-pointer walk


## [0.19.0] - 2026.09.24

### Changes

- IMM is enabled again
- Small performance improvements for RC11

### Fixes

- Minor bug and printing fixes


## [0.18.0] - 2026.09.02

### Added

- Support for weak CASes
- Support for the failure memory ordering of CASes
- Support for LLVM 21 and 22
- More build options, including the ability to link against tcmalloc (see `GENMC_TCMALLOC`)

### Fixes

- Better mixed-size access detection
- Various bug fixes
- Minor performance improvements

### Changes

- Temporary IR files now honor `TMPDIR`
- Formatting changes

### Removed

- Support for LLVM < 19


## [0.17.0] - 2026.04.08

### Added

- Build option to not add non-atomic labels to graph (`-DEMIT_NA_LABELS=OFF`; Miri only)
- VS Code extension for GenMC (work in progress)
- Mode for random exploration
- Documentation website (https://mpi-sws.github.io/genmc)

### Fixes

- Various bug fixes

### Changes

- IMM is temporarily disabled
- Manual is converted to Markdown
- Refactored/redesigned various internal primitives
- Project folders restructured

## [0.16.1] - 2025.11.28

### Fixes

- Bug fixes for Github issues


## [0.16.0] - 2025.11.28

### Fixes

- Various bug fixes
- Minor documentation update

### Changes

- Instruction caching is off by default (enabled with `-cache-instructions`)


## [0.15.1] - 2025.11.16

### Fixes

- Compatibility fixes for older compilers


## [0.15.0] - 2025.11.16
### Added

- Support for building GenMC library only (`-DBUILD_LLI=OFF`; no LLVM required)

### Fixes

- Bug fixes in config validation

### Changes

- New error-reporting and scheduling API

### Removed

- Support for LLVM-{13,14}

## [0.14.0] - 2025.11.03
### Changes

- API changes for Miri frontend


## [0.13.1] - 2025.10.11
### Added

- Experimental support for `pthread_rwlock`
- `__VERIFIER_output` primitive to add user info to printed graphs

### Fixes

- Bug fixes


## [0.13.0] - 2025.08.05
### Changes

- GenMC is now licensed under Apache/MIT

### Fixes

- Minor comments and bug fixes


## [0.12.0] - 2025.07.24
### Added

- Support for LLVM overflow intrinsics
- Experimental LLVM-based Rust frontend

### Changes

- Switched to cmake
- GenMC builds as a library as well by default

### Fixes

- Various bug fixes


## [0.11.0] - 2025.04.05
### Added
- Support for LLVM 20
- Support for (bounded) linearizability checking via Relinche
- Documentation for Relinche and state-space bounding

### Fixes

- Converted comments into doxygen comments

### Removed

- Test suite no longer depends on `getopt`
- Support for LLVM-{10,11,12}


## [0.10.3] - 2025.02.26
### Added

- Support for LLVM 19

### Changes

- Internal model for dynamic addresses
- Code structure refactoring

### Fixes

- Fix compilation warnings (thanks to mingodad)

### Deprecated

- Support for LLVM-{10,11,12}

### Removed

- Support for LLVM-{7,8,9}


## [0.10.2] - 2024.09.11
### Added

- Support for LLVM 17/18
- GenMC now warns when memory is not freed
- Minor performance improvements
- Landing page for doxygen + rudimentary development documentation

### Changed

- `__VERIFIER_assume()` and `__VERIFIER_spin_end()` now take a boolean argument (instead of int)
- GenMC now uses the new LLVM pass manager
- IMM performance may incur a minor penalty
- Project layout under `src`

### Fixes

- Minor bug fixes

## [0.10.1] - 2024.02.22
### Fixes
- Fixed in-place revisiting for CASes
- Fixed genmc include path (by Jonathan Schwender)
- Minor bug fixes

### Changed
- Verbosity level is now respected for stdout too
- GenMC is less conservative when reporting ww-races


## [0.10.0] - 2023.10.25
### Added

- Support for SC, TSO, RA memory models
- Symmetry reduction (automatic or via `__VERIFIER_spawn_symmetric`)
- In-place revisiting (from Awamoche [CAV 2023])
- Preemption/Round-Robin bounding under SC [TACAS 2023, FMCAD 2023]
- State-space size estimation before verification begins
- Performance optimizations
- Support for LLVM-16
- Rudimentary doxygen documentation

### Changed

- GenMC now requires a compiler with C++20 support

### Deprecated

- Support for LLVM-{7,8,9}

### Removed

- Support for LKMM, Persevere

### Fixes

- Various bug issues


## [0.9] - 2022.12.26
### Added

- Better C++ code support
- Performance optimizations
- Faster `configure`
- Support for LLVM-{14, 15}

### Changed

- GenMC now requires a C++14-compliant compiler

### Removed

- Support for LLVM-6.0
- Dependency on libclang-dev

### Fixes

- Bug fixes


## [0.8] - 2022.02.22
### Added

- Built-in support for hazard pointers
- Annotations for faster data-structure verification via `-helper` (experimental)
- `-no-unroll` switch to exclude functions from unrolling
- Performance optimizations

### Changed

- GenMC now runs under `-mo` by default
- Enhanced graph and DOT printing

### Removed

- Support for LLVM-{3.8, 4.0}

### Fixes

- Documentation
- Various bug fixes


## [0.7] - 2022.01.13
### Added

- Core algorithm now follows TruSt
- Support for concurrent verification (`-nthreads=<uint>`)
- Support for atomic_{nand,umax,umin,max,min}
- Support for LLVM-{12,13}

### Changed

- LAPOR is temporarily disabled

### Deprecated

- Support for LLVM-{3.8,4.0}

### Removed

- Verification of user-defined libraries

### Fixes

- Bug fixes


## [0.6.1] - 2021.07.27
### Added

- Documentation for for `__VERIFIER_spin_{start,end}`
- More accesses to freed memory are detected
- GenMC warns about execution graphs getting too large

### Changed

- Liveness checks are only allowed under `-mo` now

### Fixes

- Bug fixes


## [0.6] - 2021.05.30
### Added

- Support for LKMM (experimental)
- Native support for `pthread_barrier_t` (via BAM)
- More loops are automatically transformed into `assume` statements
- Support for liveness checks reinstated
- Documentation for new features

### Changed

- Error message for mixed-size accesses

### Fixes

- GenMC's output is now properly directed to stdout
- Proper support for `pthread_mutex_init` and `pthread_mutex_destroy`
- Various bugs


## [0.5.3] - 2020.12.06
### Added

- Support for `aligned_alloc()`

### Fixes

- Fixes for ppo calculation in IMM
- `malloc()` now returns aligned addresses
- Documentation fix and update
- Other minor bugs


## [0.5.2] - 2020.10.08
### Added

- Documentation for Persevere, LAPOR


## [0.5.1] - 2020.10.01
### Added

- Support for LLVM 11

### Removed

- Temporarily removed liveness support for fixing

### Fixes

- Build fixes for LLVM 10


## [0.5] - 2020.09.23
### Added

- Support for some unistd.h system calls
- Support for checking persistency semantics of files under ext4 (experimental)
- Race detection on automatic variables
- Checking for accessing uninitialized memory
- Symmetry reduction (unstable)
- Liveness checks (experimental)
- Support for memcpy() (experimental)
- Support for C++ (experimental)
- Support for <threads.h>
- Support for LLVM-9
- New CHANGELOG.md file to track changes in the project.

### Changed

- Names of various switches

### Removed

- Support for LLVM 3.5

### Fixes

- Various bug fixes
- Test cases under macOS
