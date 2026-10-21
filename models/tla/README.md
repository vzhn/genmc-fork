# TLA+ models of the error replay and the worker pool

Two small TLA+ models of the bookkeeping around GenMC's exploration that has
produced crashes and lost errors: the error replay a driver runs before it
prints an error, and the worker pool of `-nthreads`. They are not models of
the exploration algorithm (TruSt); they cover the scheduling and state
hand-offs around it, where the bugs were.

Each past bug is a constant of the model. With the constant on its old side
TLC finds the bug's trace; with every constant on the current side every
property holds. `scripts/check-tla.sh` runs all configurations and checks
each against the `\* EXPECT:` line in its `.cfg` file, so a model that stops
reproducing its bug, or a fixed configuration that stops passing, fails the
check. CMake registers it as the ctest `tla-models` when `tlc` is found.

```
scripts/check-tla.sh [-j workers] [models/tla/Name.cfg...]
```

needs the TLA+ tools (`tlc`, a wrapper around `tla2tools.jar`; `TLC=` names
another one). All twelve configurations take about 20 seconds with 4 workers.

## Replay.tla: the error replay

Source: `GenMCDriver::reportError()`, `initiateErrorReplay()`,
`haltErrorReplay()`, `handleExecutionEnd()`, `done()`, `initFromState()`,
`Scheduler::resetExplorationOptions()`, `calculateReplaySchedule()`,
`scheduleReplay()`, and the run loop in `lli/main.cpp`.

Before an error is printed, the driver re-executes the graph up to the
erroneous event to collect source information: it queues a replay item, and
the execution that takes it starts with a replay schedule, a po-rf
linearization of the graph's labels with the target moved last, finished
threads and non-atomic labels left out. The schedule drives the interpreter
thread by thread until it is spent; the policy takes over afterwards, and it
never picks a thread whose last label is a terminator (a block, the end of
the thread, a kill). The error is reported when the target is reached, which
ends the replay. The model keeps, per thread, the kinds of the labels
(atomic, non-atomic, then a terminator), how many are in the graph and how
many the interpreter has passed; the replay schedule; the replay target and
the buffered error; and the outcome counters. Executions follow one another
as the driver's `done()` has them: the queued replay, then (bounded) a
revisit that keeps a prefix of the graph, or under estimation a rerun from
an empty graph.

| Constant | Current side | Old side reproduces |
|---|---|---|
| `KeepTargetInSchedule` | PR #22: a non-atomic target stays in the schedule | issue #20: the target erased, a blocked thread never driven to it |
| `ReportIncompleteReplay` | PR #22: a replay ending before its target reports what it has | issue #20: the error lost, or its replay state carried into the next execution |
| `KillCheckAfterReport` | PR #31: no replay demanded for an existing kill label | PR #31: a warning whose replay targets a kill reports at the kill, which ends the replay, then the old check fails |

| Property | States |
|---|---|
| `NoInternalFailure`: the replay target is in the graph whenever a schedule is computed (`VERIFY(it != result.end())`), and no kill label fails its check | |
| `HardErrorsReported`: at the end, every hard error found was printed | |
| `ReportedAtMostOnce`: the first hard error halts, warnings print once | |

| Configuration | Expect | Meaning |
|---|---|---|
| `Replay-fixed` | pass | the current code |
| `Replay-bug20-lost` | `HardErrorsReported` | #20 with estimation off: the error is lost |
| `Replay-bug20-stale` | `NoInternalFailure` | #20 with estimation on: the next rerun's schedule fails its VERIFY |
| `Replay-bug20-fallback` | pass | PR #22's fallback alone still reports every error, as the PR states |
| `Replay-bug31` | `NoInternalFailure` | the kill check after a report at the kill |

Bounds: 2 threads, at most one access before the terminator, 3 executions.
The fixed configuration also passes with two accesses per thread (see the
PR for the state counts).

What the model leaves out: reads-from and coherence (the linearization is
per thread, which is enough for these properties), resume points (a resumed
execution has no replay schedule and is never an error replay), the eager
pop of a blocked assume read in `scheduleReplay()`, error replays with
`-disable-race-detection` (non-atomic labels kept), and errors found during
a replay at events other than the target (ignored by `reportError()`, as in
the code).

## Pool.tla: the worker pool

Source: `ThreadPool::popTask()`, `submit()`, `halt()`, `waitForTasks()`,
the worker loop in `addWorker()`, and `GenMCDriver::initFromState()`.

Workers pop tasks (executions) from a global queue. A running task may
submit new ones, find an error (which starts an error replay in that
worker's driver), or finish. The worker that reports an error halts the
pool; the others stop when they see the flag. A worker with nothing to do
sleeps on a condition variable until a submission, the last completion or
the halt; the main thread waits for the last completion or the halt. Every
mutex-protected block is one step of the model.

| Constant | Current side | Old side reproduces |
|---|---|---|
| `HaltCheckFirst` | PR #21: `popTask()` checks the halt flag before the queue | #17: a worker whose exploration stopped at a halt takes another task |
| `PredicateWait` | PR #21: the wait re-checks the queue under the mutex | #17: a submission between the unlocked pop and the wait is lost; the worker sleeps with a task queued |
| `MainWait` | `"doneCV"`: `waitForTasks()` sleeps on its own variable | `"busy"`: the yield loop before PR #21 (correct, a core wasted); `"sharedCV"`: the design PR #21 rejected, where a submission's `notify_one()` can wake main instead of a worker |
| `DropReplayOnInit` | PR #19: `initFromState()` drops a pending replay | #18: a task starts with the previous task's replay target, and the next schedule looks for an event of the old graph |

| Property | |
|---|---|
| `NoTaskAfterHalt` | no task is taken from the queue after the halt |
| `NoStaleReplay` | no task starts with a pending error replay |
| `NoSleepWithWork` | no worker goes to sleep while a task is queued |
| `NoWakeupToMain` | a submission never wakes the main thread instead of a sleeping worker |
| `Termination` | every run ends: the workers exit and `waitForTasks()` returns (checked with weak fairness) |

| Configuration | Expect | Meaning |
|---|---|---|
| `Pool-fixed` | pass | the current pool |
| `Pool-bug17-wakeup` | `NoSleepWithWork` | the lost wakeup |
| `Pool-bug17-halt` | `NoTaskAfterHalt` | a task taken after the halt |
| `Pool-bug18` | `NoStaleReplay` | the stale replay target of #18 (both fixes absent) |
| `Pool-bug18-haltfirst` | pass | PR #21 without PR #19: once no task is taken after a halt, a pending replay cannot reach `initFromState()`; either fix suffices |
| `Pool-sharedcv` | `NoWakeupToMain` | main on the workers' condition variable |
| `Pool-busy` | pass | the polling `waitForTasks()`: correct, only costly |

Bounds: 2 workers, 4 tasks. The busy wait of #17 is a cost, not a property;
the model only shows the polling version terminates too.

## Keeping the models honest

The models describe the code as of PR #104 (0.19.0-vzhn.13). When the
scheduler, the error replay, `done()`, `initFromState()` or the pool change,
change the model with them: add the new rule, keep the old one behind a
constant if it was a bug, add a configuration that expects the bug, and run
`scripts/check-tla.sh`. A configuration whose expectation stops holding is
the signal that the model and the code have parted.
