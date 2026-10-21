------------------------------ MODULE Pool ------------------------------
(***************************************************************************)
(* GenMC's worker pool (genmc/Support/ThreadPool.cpp) and the driver state  *)
(* a worker carries from one task to the next (GenMCDriver::initFromState). *)
(*                                                                         *)
(* Workers pop execution tasks from a global queue; a running task may     *)
(* submit new ones (a backward revisit handed to another worker), find an  *)
(* error (which starts an error replay in that worker's driver), or finish.*)
(* The worker that reports an error halts the pool; the others stop their  *)
(* exploration when they see the halt flag. A worker with nothing to do    *)
(* sleeps on a condition variable until a submission, the last completion  *)
(* or the halt; the main thread waits for the last completion or the halt. *)
(*                                                                         *)
(* Every mutex-protected block is one step. Each switch is a past bug      *)
(* (vzhn/genmc-fork issues #17 and #18): on its old side TLC finds the     *)
(* bug's trace, on the current side every property holds. README.md tells.*)
(***************************************************************************)
EXTENDS Naturals, FiniteSets, TLC

CONSTANTS
    NWorkers,           \* worker threads
    MaxTasks,           \* most tasks in a run, the seed task included
    HaltCheckFirst,     \* PR #21 (#17, #18): popTask() checks the halt flag before taking a task
    PredicateWait,      \* PR #21 (#17): the wait re-checks the queue under the mutex
    MainWait,           \* waitForTasks(): "busy" (before PR #21), "sharedCV" (the design
                        \* PR #21 rejected: main sleeps on the workers' variable), "doneCV"
    DropReplayOnInit    \* PR #19 (#18): initFromState() drops a pending error replay

ASSUME NWorkers >= 1 /\ MaxTasks >= 1 /\ MainWait \in {"busy", "sharedCV", "doneCV"}

Workers == 1..NWorkers

VARIABLES
    pc,              \* each worker's step
    pending,         \* whether a worker's driver holds an error replay (eventToReexecute_)
    queue,           \* tasks in the global queue
    remaining,       \* remainingTasks_: submitted and not yet finished
    submitted,       \* tasks submitted so far (bounds the run)
    halted,          \* shouldHalt_
    waiters,         \* workers asleep on stateCV_
    mainPc,          \* the main thread: "wait" (checking), "sleep", "done"
    sleptWithWork,   \* a worker went to sleep while a task was queued (lost wakeup)
    startedAfterHalt,\* a worker started a task after the pool halted
    staleReplay,     \* a task started with the previous task's error replay still pending
    wakeupToMain     \* a submission's notify_one() woke the main thread while a worker slept

vars == <<pc, pending, queue, remaining, submitted, halted, waiters, mainPc,
          sleptWithWork, startedAfterHalt, staleReplay, wakeupToMain>>
flags == <<sleptWithWork, startedAfterHalt, staleReplay, wakeupToMain>>

Init ==
    /\ pc = [w \in Workers |-> "pop"]
    /\ pending = [w \in Workers |-> FALSE]
    /\ queue = 1 /\ remaining = 1 /\ submitted = 1
    /\ halted = FALSE
    /\ waiters = {}
    /\ mainPc = "wait"
    /\ sleptWithWork = FALSE /\ startedAfterHalt = FALSE
    /\ staleReplay = FALSE /\ wakeupToMain = FALSE

MainSleeping == mainPc = "sleep"
WakeMain == IF MainSleeping THEN "wait" ELSE mainPc

(* popTask(), first part: halt check (PR #21) and the queue, under the mutex.
   A task taken while the pool is halted is the defect of #17: the worker
   starts an exploration that stops at once, with the state that #18 is about *)
Pop(w) ==
    /\ pc[w] = "pop"
    /\ IF HaltCheckFirst /\ halted
       THEN pc' = [pc EXCEPT ![w] = "exited"] /\ UNCHANGED <<queue, startedAfterHalt>>
       ELSE IF queue > 0
       THEN /\ pc' = [pc EXCEPT ![w] = "init"] /\ queue' = queue - 1
            /\ startedAfterHalt' = (startedAfterHalt \/ halted)
       ELSE pc' = [pc EXCEPT ![w] = "check"] /\ UNCHANGED <<queue, startedAfterHalt>>
    /\ UNCHANGED <<pending, remaining, submitted, halted, waiters, mainPc,
                   sleptWithWork, staleReplay, wakeupToMain>>

(* popTask(), second part: nothing was queued; exit if the pool halted or
   every task is done, else sleep. With the predicate wait the queue is
   re-checked under the mutex first, so a task queued since is not missed *)
Check(w) ==
    /\ pc[w] = "check"
    /\ IF halted \/ remaining = 0
       THEN pc' = [pc EXCEPT ![w] = "exited"] /\ UNCHANGED <<waiters, sleptWithWork>>
       ELSE IF PredicateWait /\ queue > 0
       THEN pc' = [pc EXCEPT ![w] = "pop"] /\ UNCHANGED <<waiters, sleptWithWork>>
       ELSE /\ pc' = [pc EXCEPT ![w] = "sleep"]
            /\ waiters' = waiters \cup {w}
            /\ sleptWithWork' = (sleptWithWork \/ queue > 0)
    /\ UNCHANGED <<pending, queue, remaining, submitted, halted, mainPc,
                   startedAfterHalt, staleReplay, wakeupToMain>>

(* A notified worker: the predicate wait re-checks, the old wait loops back *)
Woken(w) ==
    /\ pc[w] = "woken"
    /\ pc' = [pc EXCEPT ![w] = IF PredicateWait THEN "check" ELSE "pop"]
    /\ UNCHANGED <<pending, queue, remaining, submitted, halted, waiters, mainPc, flags>>

(* initFromState(): the task's graph replaces the worker's execution stack *)
StartTask(w) ==
    /\ pc[w] = "init"
    /\ staleReplay' = (staleReplay \/ (pending[w] /\ ~DropReplayOnInit))
    /\ pending' = IF DropReplayOnInit THEN [pending EXCEPT ![w] = FALSE] ELSE pending
    /\ pc' = [pc EXCEPT ![w] = "run"]
    /\ UNCHANGED <<queue, remaining, submitted, halted, waiters, mainPc,
                   sleptWithWork, startedAfterHalt, wakeupToMain>>

(* submit(): push and notify_one() under the mutex. The notification goes to
   a sleeping worker, or, when main shares the variable, maybe to main *)
Submit(w) ==
    /\ pc[w] = "run" /\ ~halted /\ submitted < MaxTasks
    /\ queue' = queue + 1 /\ remaining' = remaining + 1 /\ submitted' = submitted + 1
    /\ \/ /\ waiters = {} /\ ~(MainWait = "sharedCV" /\ MainSleeping)
          /\ UNCHANGED <<pc, waiters, mainPc, wakeupToMain>>
       \/ /\ \E v \in waiters:
             /\ waiters' = waiters \ {v}
             /\ pc' = [pc EXCEPT ![v] = "woken"]
          /\ UNCHANGED <<mainPc, wakeupToMain>>
       \/ /\ MainWait = "sharedCV" /\ MainSleeping
          /\ mainPc' = "wait"
          /\ wakeupToMain' = (wakeupToMain \/ waiters /= {})
          /\ UNCHANGED <<pc, waiters>>
    /\ UNCHANGED <<pending, halted, sleptWithWork, startedAfterHalt, staleReplay>>

(* The exploration of a task: it stops at a halt, finds an error, or ends *)
Explore(w) ==
    /\ pc[w] = "run"
    /\ \/ /\ halted
          /\ pc' = [pc EXCEPT ![w] = "post"]
          /\ UNCHANGED pending
       \/ /\ ~halted
          /\ pc' = [pc EXCEPT ![w] = "replay"]
          /\ pending' = [pending EXCEPT ![w] = TRUE]
       \/ /\ ~halted
          /\ pc' = [pc EXCEPT ![w] = "post"]
          /\ UNCHANGED pending
    /\ UNCHANGED <<queue, remaining, submitted, halted, waiters, mainPc, flags>>

(* The error replay: cut short by another worker's halt (the replay state
   stays in the driver), or completed: reportError() prints, haltErrorReplay()
   clears the replay, halt() stops the pool and wakes everyone *)
Replay(w) ==
    /\ pc[w] = "replay"
    /\ \/ /\ halted
          /\ pc' = [pc EXCEPT ![w] = "post"]
          /\ UNCHANGED <<pending, halted, waiters, mainPc>>
       \/ /\ ~halted
          /\ halted' = TRUE
          /\ pending' = [pending EXCEPT ![w] = FALSE]
          /\ waiters' = {}
          /\ mainPc' = WakeMain
          /\ pc' = [v \in Workers |-> IF v = w THEN "post"
                                      ELSE IF v \in waiters THEN "woken" ELSE pc[v]]
    /\ UNCHANGED <<queue, remaining, submitted, flags>>

(* After a task: the last completion wakes everyone and the worker leaves;
   otherwise it looks for the next task *)
Post(w) ==
    /\ pc[w] = "post"
    /\ remaining' = remaining - 1
    /\ IF remaining = 1
       THEN /\ waiters' = {}
            /\ mainPc' = WakeMain
            /\ pc' = [v \in Workers |-> IF v = w THEN "exited"
                                        ELSE IF v \in waiters THEN "woken" ELSE pc[v]]
       ELSE /\ pc' = [pc EXCEPT ![w] = "pop"]
            /\ UNCHANGED <<waiters, mainPc>>
    /\ UNCHANGED <<pending, queue, submitted, halted, flags>>

(* waitForTasks(): poll (busy wait), or sleep until the last completion or
   the halt. The predicate is re-checked on every wakeup *)
Main ==
    /\ mainPc = "wait"
    /\ IF halted \/ remaining = 0 THEN mainPc' = "done"
       ELSE mainPc' = IF MainWait = "busy" THEN "wait" ELSE "sleep"
    /\ UNCHANGED <<pc, pending, queue, remaining, submitted, halted, waiters, flags>>

Finished == mainPc = "done" /\ \A w \in Workers: pc[w] = "exited"
Stutter == Finished /\ UNCHANGED vars

Next ==
    \/ \E w \in Workers: Pop(w) \/ Check(w) \/ Woken(w) \/ StartTask(w)
                         \/ Submit(w) \/ Explore(w) \/ Replay(w) \/ Post(w)
    \/ Main
    \/ Stutter

Spec == Init /\ [][Next]_vars /\ WF_vars(Next)

(* No worker starts a task once the pool has halted (PR #21). *)
NoTaskAfterHalt == ~startedAfterHalt
(* No task starts with the previous task's error replay still pending (PR #19
   or PR #21: either suffices, see README). *)
NoStaleReplay == ~staleReplay
(* No worker goes to sleep while a task is queued (PR #21). *)
NoSleepWithWork == ~sleptWithWork
(* A submission never wakes the main thread instead of a sleeping worker. *)
NoWakeupToMain == ~wakeupToMain
(* Every run ends: all workers exit and waitForTasks() returns. *)
Termination == <>Finished

=============================================================================
