# Changelog

Notable changes to GenMC will be documented in this file. This
repository is only updated whenever a new version of GenMC is
released.


## [Unreleased]

- Merge of MIXER [POPL'25] into mainline GenMC

### Fixed

- `-lkmm` missed executions that LKMM allows when the events a revisit
  kept depended on the path a branch took (#77). The prefix of a write
  (what its revisits keep) had every fence before every later event
  (`[F] ; po`) and, through a same-location write, a release or a fence in
  a branch, the read the branch tests: a write after the join then could
  not revisit that read, or the revisit kept the branch's events, which the
  new path never reached, and the run was dropped. Under `-lkmm`
  (exhaustive) a revisit now keeps the model's prefix of the write without
  the events that exist because of what the revisited read read (those
  inside a branch it controls, `EventDeps::sctrl`, with their readers and
  what depends on them: `keptPrefixView()`), a read is revisitable by
  every write that does not depend on it and that the prefix orders after
  it only through those, the kat's prefix no
  longer has `[F] ; po` (vzhn/kater-fork, the checker regenerated), and a
  successful acquire CAS no longer makes every later event of its thread
  depend on its read there (the exhaustive exploration deduplicates the
  graphs #230 was about). What this opened is closed: a thread whose new
  path contradicts the events kept after it (a kept event skipped, a store
  with no place in coherence before a kept read, the thread's end) stops
  there with a block instead of dropping the run, so that the other
  threads' writes can still revisit the run's reads; the whole graph is
  checked with its views computed anew, as under `-exhaustive -imm`
  (coherence per location, not by the hb view), at a revisit that keeps
  events after the read, when an event goes before kept ones (the views
  are recomputed in place then), and when a run ends, mooted or not (a
  mooted run's queued revisits came from an unchecked graph; its own
  atomicity violation is left out); the checks between the events of a run
  leave LKMM's plain-access rules to the complete execution; an error
  found while a thread has events kept after a hole it has not filled is
  not reported from that run (a kept write may be read through a lock the
  thread has not taken again), a real one being found again with them in
  place; and a graph produced before is recognized before it is checked.
  On the 800 fuzz seeds of `scripts/lkmm/fuzz-ctrl.py` every seed herd7
  finishes now agrees (782, was 776), with `-check-invariants`; the corpus
  is unchanged; the optimal exploration (`-disable-lkmm-exhaustive`) gains
  on 6 seeds and loses none. Programs whose threads branch on shared
  state inside critical sections explore many more graphs (#261):
  `tests/correct/lkmm/kept-free-access` takes about 20 times longer, and
  two 4-lock tests of the corpus (`manual/absperf/C-SB+l-o-o-u+...`) no
  longer finish within the comparison's limit (allowlisted). Master also
  counted executions violating coherence after a
  forward revisit of a read before a kept read of its location (I5;
  `tests/correct/lkmm/kept-read-bounds-store-anew`). Tests:
  `tests/correct/lkmm/{fence-in-branch-prefix,
  kept-write-after-revisited-read, branch-release-moi-prefix,
  branch-write-moi-rmb-prefix, branch-write-moi-cas-prefix,
  dead-path-thread-stops, kept-read-bounds-store-anew(-2),
  mooted-run-pending-check}`; `cas-path-lb` documents the gap left: a
  cycle through the path a `cmpxchg()` takes, which no revisit builds.
- The debug build (`-DGENMC_DEBUG=ON`) did not compile
  (`GenMCDriver::trackExecutionBound()` used a private member).

- `-exhaustive` reported errors under `-imm` that IMM rules out. A
  revisit keeps the events the kept ones compute from, such as a store
  that a CAS's expected value reads, without the events they are merely
  ordered after, such as an acquire load before that store: those are
  executed again, and the kept events' views lacked what they then order
  before them, as did the views of the events added after them. A plain
  write after such a store raced with a write the acquire load had
  synchronized with (a false "Non-atomic race!"), and a load after it read
  a value overwritten before that write (a false "Safety violation!",
  herd7: never). The default exploration finds neither. What the views
  leave unordered is now checked again with the views computed anew, and
  an error is reported only if the graph is consistent with its views
  computed anew (`tests/correct/infr/imm-exhaustive-kept-write-views`).
- `-exhaustive` reported a false "Non-atomic race!" under `-imm` in
  `tests/correct/saver/spin-fmutex`, where the default exploration finds
  none in 1719 executions. A backward revisit keeps, with each kept
  event, the creation of its thread, and it took every thread's begin for
  a kept event (every view has it): the create of a thread that had not
  run yet was kept, without the join before it in `main`. Once the join
  was executed again, the views of the create and of the thread still
  lacked it, and the thread's accesses raced with those of the joined
  thread. A begin alone now keeps nothing; `-exhaustive` gives 1719
  executions (`tests/correct/infr/imm-exhaustive-kept-begin`).
- `-lkmm` gave events after a branch control dependencies the source does
  not have (#78): LLVM's `JumpThreadingPass`, when a later branch tests a
  value an earlier one set, moves the code after the earlier branch's join
  into its arms. The pass is off under `-lkmm`; fuzz seed 155 of
  `scripts/lkmm/fuzz-ctrl.py` now has herd7's 250 executions (was 170).
  `scripts/lkmm/lkmm-diff.py` translates litmus registers as locals under
  `--model lkmm`, as only a local carries a register's dependencies
  (`--global-regs` for the old translation): fuzz seeds 0-799 754 -> 757
  agree, the corpus loses `C-viro-2020.09.29a` (a value a branch sets is
  given the branch's dependencies whichever path it came by, #77,
  allowlisted) (`tests/correct/lkmm/jump-threading-ctrl`).
- In a program with thread-locals, the end of the main thread was taken
  for the end of every thread-local: an access of another thread not
  ordered before main's end, to any location, was reported as "Attempt to
  access freed memory", depending on the schedule (`print-names-thread-local`
  under some arbitrary schedules). The main thread's copies last as long
  as the program, and its end frees nothing
  (`tests/correct/infr/tls-main-ends-first`).
- `-lkmm` missed executions that LKMM allows (#77): under the kernel
  model, the events that order an event after a branch depend on the
  path the branch took (a write in it to the same location, a fence),
  and the maximality check of the optimal exploration assumes they do
  not, so the only revisit that builds an execution could be rejected.
  `-lkmm` now explores exhaustively by default (every consistent revisit,
  graphs deduplicated, as `-exhaustive`), which finds herd7's executions
  in these cases: 6 more of the 800 fuzz seeds of
  `scripts/lkmm/fuzz-ctrl.py` agree with herd7, the litmus corpus and the
  kernel's litmus tests are unchanged, and the exploration does about a
  third more work on the corpus's largest tests. It runs single-threaded
  and without load annotations or estimation; `-disable-lkmm-exhaustive`
  restores the optimal exploration, and so does `-disable-spin-assume` (a
  spin loop left as a loop has no end under the exhaustive exploration)
  (`tests/correct/lkmm/prefix-path-revisit`;
  `dup-kept-instance` and `inverted-co-range` now have herd7's counts,
  4 and 2926, whatever the schedule).
- `-lkmm` explored executions that LKMM forbids, and reported errors in
  them: the write of a `cmpxchg()` (or another RMW) whose read did not
  read the last write in coherence was placed after that write without
  checking the graph, which only a write placed last can skip. Through its
  `coe` edges, `prop` could close an hb cycle that the check of the read
  part could not see: `r0 = READ_ONCE(y); if (r0) r1 = cmpxchg_relaxed(&x,
  0, 2);` against `WRITE_ONCE(x, 1); smp_store_release(&y, 2);` reported
  that `r0 == 2 && r1 == 0` can happen (herd7: never). Such a write is now
  checked under `-lkmm`; fuzz seeds 101 and 496 of
  `scripts/lkmm/fuzz-ctrl.py` now have herd7's counts (3479, 554; were
  3756, 734) (`tests/correct/lkmm/ctrl-rmw-coe-cycle`).
- A spin loop whose condition reads several locations, such as
  `while (x == 0 || y == 0)`, did not revisit its later reads in place
  (#92): the loop's failing assumption reached only the first branch
  before it, so only the read of `x` was annotated. An iteration that
  failed at `y` stayed blocked: each later write to `y` revisited the
  read in a copy of the execution, and the blocked execution was explored
  to its end as well. Every branch to the assumption now gets it
  (`tests/correct/saver/spin-load-or`: no blocked execution, one before).
- Nor did a spin loop that retries with `continue`, or by reaching the
  end of its body, revisit its reads in place (#92), as in a seqlock
  reader (`if (s & 1) continue; ... if (seq == s) break;`): such branches
  reach the latch, where the failing assumption is, through blocks of
  their own that only jump on, and the assumption did not get past them.
  It now does (`tests/correct/saver/spin-seqlock-reader`: 5 blocked
  executions, 13 before; a relaxed read of the data is still reported:
  `tests/wrong/safety/seqlock-reader-relaxed-data`).
- A read whose spin-loop condition takes a signed remainder, as in
  `while (seq % 2 == 1)` on an `int`, was not annotated either (#92): the
  annotator handled the unsigned remainder only, and such a read was not
  revisited in place (`tests/correct/saver/spin-load-srem`: no blocked
  execution, two before). With the three fixes, Dat3M's seqlock
  benchmark with three readers explores 41,875 blocked executions instead
  of 360,057, with 9 times fewer instructions; the remaining ones are
  failed iterations that no later write can make succeed, for each
  combination of the other readers' executions.
- With `-disable-race-detection`, GenMC missed the errors of racy
  programs under `-rc11`, `-imm` and `-sc` (#142): a non-atomic read read
  only the latest write, and a non-atomic write went last in coherence,
  which loses no execution only as long as races are reported. A plain
  `if (flag) assert(data == 1)` racing with `data = 1; flag = 1` was
  explored once and reported no errors (`-lkmm` found the violation).
  Without race detection, non-atomic accesses are now explored like
  atomic ones, as under `-lkmm`; programs without races have the same
  executions as before (`tests/wrong/safety/plain-race-ignored`,
  `tests/correct/infr/plain-race-ignored`). The memory-model detector
  also no longer moves such a program to a stronger model, such as SC
  for a program without atomics, since that keeps the outcomes of
  race-free programs only (`scripts/check-race-detection-model.sh`, ctest
  `race-detection-model`).
- Whether GenMC started over without symmetry reduction (SR) or in-place
  revisiting (IPR), which unordered writes make an error, depended on the
  schedule (#130): `tests/correct/synthetic/wainc` kept SR, and explored
  N+1 executions instead of (N+1)!, under an arbitrary schedule. Three
  causes: unordered writes found before the first symmetric thread was
  created never made SR an error, as they do when found after it; once
  one unordered write had been reported, later ones were not checked (a
  `fetch_add()` unordered with a store to a location IPR revisits kept
  IPR on); and `-disable-race-detection` skipped the check altogether.
  The check now runs at every write while SR or IPR is on, with or
  without race detection, and creating a symmetric thread after unordered
  writes makes SR an error too (`scripts/check-unordered-writes-fallback.sh`,
  ctest `unordered-writes-fallback`, with
  `tests/correct/sr/sym-readers-after-ww` and
  `tests/correct/infr/ipr-ww-rmw-after-store`).
- An installed GenMC compiled programs with the runtime headers of the
  source tree it was built from, whatever that tree had become since, as
  long as it existed; once it was gone, C++ programs no longer compiled,
  because the C++ runtime headers (`atomic`, `cassert`, `cstdio`,
  `cstdlib`, `thread`, which have no extension) were never installed.
  Every runtime header is installed now, and an installed binary uses
  only the headers installed with it, found from its own directory (a
  moved prefix works too); a binary run from its build tree uses the
  source tree's, as before (`scripts/check-install-headers.sh`, ctest
  `install-headers`).
- `__VERIFIER_nondet_int()` returns the same pseudo-random sequence in
  every execution, so GenMC explores one value per call, and nothing said
  so: `int x = __VERIFIER_nondet_int(); assert(x != 1);` reported "No
  errors were detected" after one execution (#48). GenMC now warns, once
  per run and before exploring, when the program calls such a function:
  the warning names the functions called and says that a run that finds
  no errors does not cover their other values. Each call still returns
  one value (`scripts/check-nondet-warning.sh`, ctest `nondet-warning`).
- Error reports located the events of the runtime headers' wrappers at
  the header's line, not the program's (#143): a `pthread_mutex_lock()`,
  `free()` or `pthread_create()` showed as `pthread.h:401`, `stdlib.h:29`
  or `pthread.h:148`, in the graph and in `-print-error-trace`, so a
  deadlock or a use of freed memory did not say which call took the lock
  or freed the block, and the block of a thread stuck at a lock had no
  location at all. Such an event is now reported at the program's line
  that calls the header (`locs.c:8`), found through the locations the
  wrapper was inlined at or the calls on the interpreter's stack; this
  covers `std::atomic`, and a wrapper that calls another one
  (`pthread_cond_wait()`). A block after a lock is reported at the lock's
  line. A block the error replay does not re-execute, such as that of a
  `pthread_join()` waiting for a stuck thread, still has no location
  (`scripts/check-call-site-locs.sh`, ctest `call-site-locs`).
- Error traces gave the fields of stack variables no name: an access to
  `loc.b` of a local `struct pair loc` printed as `(, 1)`. LLVM (19 and
  later) keeps a variable's `dbg.declare` as a debug record attached to
  an instruction, which the collection of names did not look at; it reads
  both forms now (`tests/wrong/infr/print-names-local`).
- Error traces gave heap blocks no name (#144): a field of a `malloc`'d
  or `calloc`'d object printed as `(, 1)`. A heap block is now named after
  the program's call that allocated it, numbered if that call made
  several (`calloc@locs.c:18`, `new@list.cpp:7#2`), with the fields of
  the type its result is stored as when known: the pointee type of the
  local or global variable it is stored to, or of the function that
  returns it (`calloc@locs.c:18.mutex.__private`). A block larger than
  that type is an array of it (`calloc@a.c:12[2].next`); a block of
  unknown type shows the offset (`malloc@a.c:30+8`)
  (`tests/wrong/infr/print-names-heap`, `tests/wrong/infr/print-names-heap-new`).
- `-print-error-trace` left out the event right after a thread's
  creation, taking it for the store of the new thread's ID: when the
  `pthread_t` was kept in a register, which has no such store, a second
  `pthread_create()` or a `pthread_join()` that followed was missing from
  the trace. Only a write at the creation's line is left out now
  (`tests/wrong/infr/trace-after-create`).
- The SV-COMP prelude for tasks that include `<pthread.h>`
  (`scripts/svcomp/svcomp-shim.h`) no longer compiled since `pthread.h`
  got `pthread_once()` (#154): it defined its own `pthread_once_t` and
  `pthread_once()`, so every such task was a compile error. It now uses
  those of `pthread.h`, which work the same way.
  `scripts/check-svcomp-shim.sh` (ctest `svcomp-shim`) runs
  `svcomp-run.py` on small tasks (`scripts/svcomp/selftest`) through both
  preludes, the one for preprocessed tasks included, and checks their
  verdicts.
- `scripts/svcomp/svcomp-run.py` verified a task's `.c` file, while SV-COMP
  verifies its `input_files`, which for 745 of the 1154 concurrency tasks
  is the preprocessed `.i`. The `.c` of some tasks includes headers the task
  does not ship (`svcomp.h` in `pthread-driver-races`), and 32 tasks have no
  `.c` at all. The runner now verifies the input file. Its self-test has a
  task whose `.c` does not build (`scripts/svcomp/selftest/input-file`).
  On the `.i` files, the tail that `svcomp-run.py` appends to preprocessed
  tasks fell short of `svcomp-shim.h`: it took a call checked in
  `if (pthread_create(...)) {` for a definition and left the function out,
  ignored the mutex type, so a recursive mutex deadlocked on its second
  lock and a reachable error was missed, and had no rwlocks or
  thread-specific data. It now tells calls from definitions and has all
  three, as `svcomp-shim.h` and `pthread.h` do (self-test tasks `pp-*`).

- `__thread` and `_Thread_local` variables lived outside the execution
  graph, in a map per thread from address to value, with every thread's
  copy at the same address (#90). A thread-local struct or array aborted
  GenMC before exploration (`Constant unimplemented for type`), and a
  thread-local pointer initialized with the address of a global laid out
  after it, such as a string literal, pointed outside GenMC's memory. A
  pointer to another thread's thread-local reached the accessing thread's
  own copy: publishing a thread-local node, as lock implementations do
  (e.g. Dat3M's `ticket_awnsb_mutex`), gave false assertion violations,
  and a race through such a pointer went unreported. Each thread now has
  its own copy of every thread-local, at an address of its own in static
  memory, starting as the initializer, and accesses to it are events like
  any other: they are checked for races, and carry dependencies under
  `-imm` and `-lkmm`. An error trace names another thread's copy after
  its thread (`tlx of thread 1`). Symmetric threads stay symmetric, but a
  parent that accesses its own thread-locals between two spawns makes
  them not symmetric, as the thread-local may be published
  (`tests/correct/infr/{tls-aggregate,tls-init-address,tls-published}`,
  `tests/wrong/racy/tls-published-race`,
  `tests/wrong/infr/print-names-thread-local`,
  `tests/correct/sr/{tls-symmetric,tls-between-spawns}`; the canonical
  graph hash of `-exhaustive` names the copies by their threads' creation
  paths: `GraphHashTest.ThreadLocalCopiesAreNamedByCreationPath`).
- `memset()` on a thread-local, or `memcpy()` between two thread-locals,
  aborted GenMC (`Invalid call to memset()!`, or `memcpy()`): clang takes
  the address of a thread-local through `@llvm.threadlocal.address`,
  behind which the promotion of memory intrinsics did not find the
  variable's type, so it left the call whole, which GenMC cannot run. It
  looks through the intrinsic now, and the call becomes an access per
  field (`tests/correct/infr/tls-memintrinsics`).
- An access to a thread's thread-local by another thread, through a
  pointer the thread published, was not reported when it came after the
  thread ended (e.g., after joining it) or concurrently with its end,
  which natively is a use after free. A thread's copies of thread-locals
  now end with it, as if freed by its end: such an access is reported as
  an access to freed memory, with the thread's end as the conflicting
  event. The main thread's copies last as long as the program
  (`tests/wrong/memory/tls-access-ended`).
- The `-sc` expectations of 15 correct tests were stale, and so they failed
  under `-sc`, which `scripts/driver.sh` does not run by default. Symmetry
  reduction now covers threads spawned into a local `pthread_t` array or
  with an argument they never read, and a loop that waits under a mutex is
  a spinloop, but only the `-rc11` and `-imm` counts were updated. No
  execution is lost under SC: with `-disable-sr`, and under `-exhaustive`,
  every test has its old count, and the new counts are those of `-rc11`
  (e.g., `saver/loop-peel2`: two readers each read 0 from the initial
  value or from the writer's first write, 4 executions, 3 up to swapping
  the readers). `scripts/sc-driver.sh` (ctest `sc-driver`) runs the
  correct tests under `-sc` now.
- The test driver skipped the last line of `args.<model>.mo.in` or
  `expected.<model>.mo.in` when it had no trailing newline (#176), and so
  did `scripts/oracle-diff.sh` for `args.<model>.mo.in`: with a single such
  line the test never ran under the model. `litmus/MPU+rel+acq`,
  `litmus/ori` and `litmus/psc-base-notin-ar` never ran, `infr/caching`,
  `data-structures/qu-opt` and three more litmus tests never ran under
  `-imm`, `saver/spin-cas1` never ran under `-rc11`, and nor did the last
  configuration of `synthetic/barrier3` under `-sc`. They run now and pass
  (`psc-base-notin-ar` did not compile: it declared `__VERIFIER_assume()`,
  a macro of `genmc.h`) (`scripts/check-driver-last-line.sh`, ctest
  `driver-last-line`).

### Performance

- `-lkmm` explores fewer graphs on programs whose threads branch on
  shared state inside critical sections, which #77's fix had made much
  slower (#261). That fix let a write revisit a read that its prefix holds
  only through the events of a branch the read controls. Such a revisit is
  now taken only when the read reaches the write through an event its
  thread keeps after the branch: the path-dependent prefix #77 was about.
  When it reaches the write through another thread's read of a branch's
  event instead (a lock taken after the read's thread released it inside
  the branch), that read can read something else, and the graphs where it
  does hold the revisit without that path. A read of an RMW whose write
  another thread's CAS reads is still revisited, as the graphs where the
  CAS reads something else can be cut short (#264). The whole graph is
  checked with its views computed anew only at a revisit or at the end of
  a run that has events kept after holes or after the read, as before #77
  otherwise. The part of LKMM's `prop` that comes back to a thread finds a
  thread's lock handovers in one pass and reuses each event's cumul-fence
  step within a check. `tests/correct/lkmm/kept-free-access` takes 9 s
  (45 s before, 3 s before #77), `tests/correct/gobench/cockroach10790`
  under `-lkmm` 198 s (over 400 s, 66 s), and the `lkmm` and `infr` tests
  91 s in all (150 s). The 3- and 4-lock `C-SB` tests of the corpus are
  unchanged, waiting on #264.

### Added

- `-print-addresses`: the printed graphs (the error graph,
  `-dump-error-graph`, `-print-exec-graphs`) show the address of each
  access, as in `Wna (@H#(0, 52), 0)`, and the block each allocation and
  free spans, as in `MALLOC  @H#(0, 48) size=8` and `FREE @H#(0, 48) size=8`.
  A heap access was printed with an empty name, or one guessed from the
  allocation site, so a tool reading the graph could not tell which block
  an access or a free touches. Off by default: the graphs are unchanged
  without it (`scripts/check-print-addresses.sh`, ctest `print-addresses`).
- The other integer `__VERIFIER_nondet_X()` of SV-COMP (#48): `bool`,
  `char`, `uchar`, `short`, `ushort`, `uint`, `unsigned`, `long`, `ulong`,
  `longlong`, `ulonglong`, `size_t`, `int128` and `uint128`, declared in
  `genmc.h`, and `u32`, `loff_t` and `sector_t`, which a program declares
  itself. Before, a call to any of them stopped GenMC with "Tried to
  execute an unknown external function". Like `__VERIFIER_nondet_int()`,
  each call returns one value (an `int` of the same pseudo-random sequence,
  fitted to the type), and GenMC prints the same warning. The `float`,
  `double` and pointer variants are still unsupported
  (`tests/correct/infr/nondet-variants`).


## [0.19.0-vzhn.14] - 2026.10.22

Fork release on top of 0.19.0 (https://github.com/vzhn/genmc-fork).

### Fixed

- `-imm` explored executions that IMM forbids through a `fetch_add`, an
  `exchange` or another read-modify-write (#156): IMM orders the read of
  an RMW, an exclusive read, before all later events of its thread
  (`[Rex] ; po`), GenMC's IMM only before the RMW's write part. A
  load-buffering cycle through a relaxed `fetch_add` was consistent, and
  an assertion that holds under IMM was reported as violated (as in
  upstream GenMC). The read of an RMW now orders those events, as a CAS's
  does. On fuzz seeds 249, 637 and 1070 the default exploration no longer
  disagrees with `-exhaustive` or depends on the schedule: 55, 460 and
  4123 executions, as under rc11.
- Under `-imm`, the read of a CAS is ordered before the later events of its
  thread whatever branches follow the CAS, as IMM orders it (`[Rex] ; po`,
  a failing CAS's read included). Before, the ordering came only from the
  control dependency of the branch storing the expected value. Between
  vzhn.13 and this release (#133), a branch with one target, which is what
  JumpThreadingPass leaves when that store is dead, stopped opening one,
  and `-imm` crashed and lost and repeated executions (#145, #137, #155).
  `-exhaustive` also stopped counting load-buffering executions through a
  failing CAS.
- A spinloop nested in another one restarted the outer loop's spinloop
  detection: the outer loop's next iteration was judged by what followed
  the inner loop's last spin start only (#147). An outer iteration whose
  side effects came before the inner loop was pruned as a repeat, which
  hid executions (with `-check-liveness`, or with an inner loop checked at
  run time, e.g. one that polls under a mutex), and `-check-liveness`
  reported a non-terminating spinloop for a retry loop around a wait loop
  that ends. Each loop is now tracked on its own, and its iteration
  includes those of the loops nested in it.
- A spin loop whose check before the loop reads with a different memory
  order than the loop's read (`w = load_relaxed(&f); while (w) w =
  load_acquire(&f);`) was not recognized as spinning, and GenMC did not
  terminate on it (#91). The loads feeding a header PHI no longer have to
  share an ordering: each keeps its own, and an iteration is blocked only
  if it leaves the PHI unchanged.
- A logical negation in an annotation expression always evaluated to 0
  (`SExprEvaluator::visitNotExpr()` negated the operand's pointer, #64).
  Loop jump threading never inverted a loop whose header leaves it on the
  true side of its branch (`for (;;) { if (r) break; r = ...; }`), so the
  spin-assume pass did not recognize such a loop and GenMC did not
  terminate on it. With negations working, the pass took a header it could
  not annotate (one with a CAS in it) for one that always enters the body,
  and inverting it broke the module: a header that touches memory now has
  no annotation, and its loop is left alone.
- `-lkmm`: a run whose thread reached, at a hole, an instruction whose kept
  event lies behind kept events of a branch the thread does not take went
  on until the thread's end instead of being dropped at once (#109). It added a
  second event for the instruction, placed it in coherence against the
  kept events, could report a spurious write-write race (against a hole),
  and queued revisits whose graphs contradict coherence; `-lkmm-reference`
  stopped on them with "The LKMM checker and the model disagree".
- Error traces printed wrong names for struct fields. With a constant
  initializer, the global's LLVM type is not the declared one: the
  padding after a `pthread_mutex_t`/`spinlock_t` or `int` member becomes
  a field (`v.b` for `v.a`), an array of such structs becomes a packed
  struct (`arr` for `arr[1].a`), bitfields are split. Without one, a field
  after implicit padding was named as if the padding were not there
  (`go_chans[0].sendq[0].sg` for `go_chans[0].nrecv`). Names are now laid
  out from the debug-info type alone (member offsets, array subranges),
  which also names base-class and anonymous-member fields without an
  empty component (`d.b1`, not `d..b1`). A wrong test's
  `expected-output.in` lists lines its output must contain
  (`tests/wrong/infr/print-names-init-padding`).
- GenMC failed before exploring a program with a zero-sized global or a
  global that is only declared. A zero-sized one (an empty struct, a
  zero-length array, or the `external global [0 x ptr]` vtable of
  `__cxxabiv1::__class_type_info` that the type info of any C++ class with
  virtual methods refers to) failed the check `size != 0` in
  `SAddrAllocator::allocate()`; a declared one (`extern int optind;`)
  crashed re-initializing it from its missing initializer. A zero-sized
  global now gets one byte, reading as zero, and a declared one keeps the
  contents of the host symbol the interpreter bound it to
  (`tests/correct/infr/{cpp-vtable-global,global-zero-size,global-extern-decl}`).
- A weak CAS lost executions in three ways: its spurious-failure flag
  survived into the forward revisits that followed (#116); the failure
  revisit of a CAS that a backward revisit made succeed cut the write it
  reads from (an internal failure under rc11 and imm, #117); and its
  spurious failure reading a write that a settled RMW reads was never
  explored (#118).
- `-imm` segfaulted when a revisit kept a `fetch_add`'s write part without
  its read part (#120), and left that write stale (an inconsistent
  execution) or relocated it when the read was executed anew (#122).
- `-imm` and `-lkmm` lost and repeated executions when a backward revisit
  deleted an access of a thread and kept later accesses of it (#121, #128,
  and #119's losses): a deleted access was judged by whether it had been
  added maximally, against the graph of the time, and a kept write whose
  prefix reaches the thread past the access did not count as an
  alternative to it. Deleted accesses are now judged against the writes
  they can reach in the revisited graph (#146).
- `-imm` lost executions with resume points, the default since vzhn.13
  (#111; fuzz seed 562: 518 of 540): an execution resumed from a saved
  state had no replay schedule, so the holes before kept events were not
  filled first (#126).
- A `malloc(0)` block live across a revisit was not registered again when
  the allocation map was rebuilt, and freeing it aborted, under every
  model (#115, #126).
- `free()` of a global, a stack variable, a wild pointer or a pointer into
  a heap block was an internal failure, a different error at a later event,
  or no error: it is now reported at the `free()` as "Attempt to free
  non-allocated memory", with the reason (#139, #140).
- `-lkmm` warned "Unordered writes" about writes that a grace period
  orders (a write in an RCU read-side critical section and one after
  `synchronize_rcu()`, #113, #114).
- `-lkmm`: a conditional branch with one target, which JumpThreadingPass
  leaves where a compare-exchange's expected value is dead, opened a
  control dependency, ordering after a failed `cmpxchg_relaxed()` what
  LKMM does not (#125, #133).
- The annotation-expression evaluator gave the minimum value back, not
  its convention 0, for a signed division overflow (`min / -1`) at widths
  below 64 (#107, #108).

### Added

- `pthread.h` has `pthread_once()`, thread-specific data
  (`pthread_key_create()`, `_delete()`, `pthread_getspecific()`,
  `pthread_setspecific()`), `pthread_rwlock_trywrlock()` and
  `pthread_atfork()` (#154). A `pthread_once()` caller that finds the init
  routine running waits until it returns. Keys are never reused, and their
  destructors are not called. As GenMC has no `fork()`, a program runs the
  fork handlers itself with `__VERIFIER_atfork_prepare()`, then
  `__VERIFIER_atfork_parent()` or `__VERIFIER_atfork_child()`
  (`tests/correct/infr/pthread-{once-key,rwlock-trywrlock,atfork}`).
- The expression evaluator's arithmetic is in `genmc/Support/SExprOps.hpp`,
  as functions of the operands' values and width; `scripts/prove-sexpr-ops.sh`
  proves them equal to LLVM's integer semantics for all inputs with Alive2.
- `-exhaustive`, a reference exploration for small programs: every
  consistent backward revisit is explored, graphs are deduplicated by a
  canonical hash (threads named by their creation path), and every
  reduction is off. `scripts/oracle-diff.sh` compares the default
  exploration with it over the test suite (ctest target `oracle-diff`),
  and `scripts/fuzz-c11.py` over random C11 programs, under arbitrary
  schedules and without resume points too. Under `-imm` it keeps
  what the events a revisit keeps depend on, and checks each revisited
  graph and each execution for consistency (#127, #135, #136).
- `-check-invariants` (off by default, on in `ENABLE_GENMC_DEBUG` builds):
  runtime checks of graph restriction, error replay, the allocation map
  rebuilt at each execution start, and the states forward revisits resume
  from; a violation is an internal failure naming the invariant (#126,
  `scripts/check-invariants-fast.sh`, ctest `invariants-fast`).
- The pass checker checks under `-imm` and `-lkmm` that every pass keeps
  the dependencies of every event (`scripts/check-pass-deps.sh`, ctest
  `check-pass-deps`), with a hidden `-disable-jump-threading` (#133).
- TLA+ models of the error replay and the worker pool, with a TLC
  configuration reproducing each past bug there (`models/tla`,
  `scripts/check-tla.sh`, ctest `tla-models` when `tlc` is found, #112).
- Unit tests of the expression evaluator and the RMW operations against
  LLVM's `APInt` and `atomicrmw`, and `scripts/prove-rmw-ops.sh`, which
  proves the RMW operations with Alive2 (#108).


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
