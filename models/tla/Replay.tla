----------------------------- MODULE Replay -----------------------------
(***************************************************************************)
(* GenMC's error replay, as one driver runs it                             *)
(* (genmc/Verification/Core/GenMCDriver.cpp, Scheduler.cpp, lli/main.cpp).      *)
(*                                                                         *)
(* Before an error is printed, the driver re-executes the graph up to the  *)
(* erroneous event to collect source information. It queues a replay item; *)
(* the execution that takes it starts with a replay schedule (a po-rf      *)
(* linearization of the graph's labels, the target moved last, finished    *)
(* threads and non-atomic labels left out), which drives the interpreter   *)
(* thread by thread until it is spent; the scheduling policy takes over    *)
(* afterwards, and it never picks a thread whose last label is a           *)
(* terminator (a block, the thread's end, a kill). The error is reported   *)
(* when the interpreter reaches the target again, which ends the replay.   *)
(*                                                                         *)
(* The graph is abstracted to the kind of each label, per thread; reads-   *)
(* from and coherence play no part in replay scheduling beyond the         *)
(* linearization, for which program order per thread suffices here.       *)
(*                                                                         *)
(* Each switch below is a past bug (vzhn/genmc-fork): with the switch on   *)
(* its old side TLC finds the bug's trace, with all of them on the current *)
(* side every property holds. See models/tla/README.md.                    *)
(***************************************************************************)
EXTENDS Naturals, Sequences, FiniteSets, TLC

CONSTANTS
    NThreads,               \* threads of the program
    MaxBody,                \* most accesses per thread before its terminator (0..2)
    MaxExecs,               \* executions per run: bounds reruns and revisits
    KeepTargetInSchedule,   \* PR #22 (issue #20): a non-atomic target stays in the replay schedule
    ReportIncompleteReplay, \* PR #22 (issue #20): a replay that ends before its target reports anyway
    KillCheckAfterReport    \* before PR #31: an existing kill label demanded an ongoing replay,
                            \* checked after the report at that label ended the replay

ASSUME MaxBody \in 0..2 /\ NThreads >= 1 /\ MaxExecs >= 1

Threads == 1..NThreads
Body == {"A", "NA"}              \* an atomic or a non-atomic access
Terms == {"End", "Blk", "Kill"}  \* the thread finished, blocked, or was killed
NoEvent == [t |-> 0, i |-> 0]
NoReplay == [pos |-> NoEvent, sev |-> "none"]

Bodies == {<<>>} \cup (IF MaxBody >= 1 THEN {<<a>> : a \in Body} ELSE {})
                 \cup (IF MaxBody >= 2 THEN {<<a, b>> : a \in Body, b \in Body} ELSE {})
ThreadExecs == {Append(b, k) : b \in Bodies, k \in Terms}
Execs == [Threads -> ThreadExecs]

VARIABLES
    mode,         \* "verify", or "estimate": random reruns from an empty graph
    exec,         \* the labels each thread produces in this execution
    glen,         \* how many of them are in the graph
    pos,          \* how many the interpreter has executed in this execution
    sched,        \* the replay schedule
    target,       \* the event an error replay re-executes up to (eventToReexecute_)
    buffered,     \* the error waiting for its replay: "none", "hard" or "soft"
    pending,      \* a queued replay item (ReplayForwardRevisit)
    moot,         \* the execution was discarded
    halted,       \* a hard error was reported
    detected,     \* hard errors found
    warned,       \* warnings found
    reportedHard, \* hard errors printed
    reportedSoft, \* warnings printed
    failure,      \* an internal check that failed, or "none"
    phase,        \* "start", "run", "end", "next", "done"
    nexec         \* executions started

vars == <<mode, exec, glen, pos, sched, target, buffered, pending, moot, halted,
          detected, warned, reportedHard, reportedSoft, failure, phase, nexec>>
counters == <<detected, warned, reportedHard, reportedSoft>>

Ev(t, i) == [t |-> t, i |-> i]
Kind(e) == exec[e.t][e.i]
InGraph(e) == e.t \in Threads /\ e.i >= 1 /\ e.i <= glen[e.t]
Last(t) == IF glen[t] = 0 THEN "none" ELSE exec[t][glen[t]]
Finished(t) == Last(t) \in {"End", "Kill"}
InReplay == target /= NoEvent
PolicySchedulable(t) == pos[t] < Len(exec[t]) /\ Last(t) \notin Terms

(* calculateReplaySchedule(): the graph's labels in a po-respecting order
   (thread by thread suffices here), block labels left out; the target moved
   last; finished threads left out unless an error is replayed; non-atomic
   labels left out, the target excepted since PR #22. *)
ThreadLabels(t) == [i \in 1..glen[t] |-> Ev(t, i)]
RECURSIVE Cat(_)
Cat(n) == IF n = 0 THEN <<>> ELSE Cat(n - 1) \o ThreadLabels(n)
Linear == SelectSeq(Cat(NThreads), LAMBDA e: Kind(e) /= "Blk")
WithTarget == IF ~InReplay THEN Linear
              ELSE Append(SelectSeq(Linear, LAMBDA e: e /= target), target)
WithoutFinished == IF InReplay THEN WithTarget
                   ELSE SelectSeq(WithTarget, LAMBDA e: ~Finished(e.t))
Schedule == SelectSeq(WithoutFinished,
                      LAMBDA e: Kind(e) /= "NA" \/ (KeepTargetInSchedule /\ e = target))

(* scheduleReplay(): entries the interpreter has passed are popped lazily *)
RECURSIVE Drop(_)
Drop(s) == IF s = <<>> THEN <<>>
           ELSE IF pos[Head(s).t] >= Head(s).i THEN Drop(Tail(s)) ELSE s

Init ==
    /\ mode \in {"verify", "estimate"}
    /\ exec \in Execs
    /\ glen = [t \in Threads |-> 0]
    /\ pos = [t \in Threads |-> 0]
    /\ sched = <<>>
    /\ target = NoEvent /\ buffered = "none" /\ pending = NoReplay
    /\ moot = FALSE /\ halted = FALSE
    /\ detected = 0 /\ warned = 0 /\ reportedHard = 0 /\ reportedSoft = 0
    /\ failure = "none"
    /\ phase = "start"
    /\ nexec = 0

(* handleExecutionStart(): resetExplorationOptions() computes the schedule;
   calculateReplaySchedule() VERIFYs that the target is in the graph *)
StartExec ==
    /\ phase = "start"
    /\ IF InReplay /\ ~InGraph(target)
       THEN /\ failure' = "StaleTarget"
            /\ phase' = "done"
            /\ UNCHANGED <<mode, exec, glen, pos, sched, target, buffered, pending,
                           moot, halted, counters, nexec>>
       ELSE /\ sched' = Schedule
            /\ pos' = [t \in Threads |-> 0]
            /\ moot' = FALSE
            /\ nexec' = nexec + 1
            /\ phase' = "run"
            /\ UNCHANGED <<mode, exec, glen, target, buffered, pending, halted,
                           counters, failure>>

(* reportError() at the target: print, haltErrorReplay(), halt() if hard *)
Report(sev) ==
    /\ reportedHard' = IF sev = "hard" THEN reportedHard + 1 ELSE reportedHard
    /\ reportedSoft' = IF sev = "soft" THEN reportedSoft + 1 ELSE reportedSoft
    /\ target' = NoEvent
    /\ buffered' = "none"
    /\ halted' = (halted \/ sev = "hard")

(* The interpreter executes thread t's next instruction: an existing label
   is re-added (updateErrorInfoAndMaybeExit() reports at the target), a new
   one is added and may be found erroneous (initiateErrorReplay()) *)
Run(t) ==
    LET i == pos[t] + 1
        k == exec[t][i]
        e == Ev(t, i)
        existing == i <= glen[t]
        hit == existing /\ e = target /\ buffered /= "none"
    IN
    /\ pos' = [pos EXCEPT ![t] = i]
    /\ IF hit THEN
           /\ Report(buffered)
           /\ moot' = TRUE
           /\ failure' = IF k = "Kill" /\ KillCheckAfterReport THEN "KillCheck" ELSE failure
           /\ UNCHANGED <<glen, pending, detected, warned>>
       ELSE IF existing THEN
           UNCHANGED <<glen, target, buffered, moot, halted, counters, pending, failure>>
       ELSE
           /\ glen' = [glen EXCEPT ![t] = i]
           /\ \/ UNCHANGED <<pending, moot, detected>>
              \/ /\ k \in Body /\ ~InReplay /\ pending = NoReplay /\ ~halted
                 /\ pending' = [pos |-> e, sev |-> "hard"]
                 /\ moot' = TRUE
                 /\ detected' = detected + 1
           /\ UNCHANGED <<target, buffered, halted, warned, reportedHard, reportedSoft, failure>>

RunReplay ==
    /\ phase = "run" /\ ~moot /\ ~halted
    /\ LET s == Drop(sched) IN
       /\ s /= <<>>
       /\ sched' = s
       /\ Run(Head(s).t)
    /\ UNCHANGED <<mode, exec, phase, nexec>>

RunPolicy ==
    /\ phase = "run" /\ ~moot /\ ~halted
    /\ Drop(sched) = <<>>
    /\ \E t \in Threads: PolicySchedulable(t) /\ Run(t)
    /\ sched' = <<>>
    /\ UNCHANGED <<mode, exec, phase, nexec>>

EndExec ==
    /\ phase = "run"
    /\ moot \/ halted \/ (Drop(sched) = <<>> /\ ~\E t \in Threads: PolicySchedulable(t))
    /\ phase' = "end"
    /\ UNCHANGED <<mode, exec, glen, pos, sched, target, buffered, pending, moot, halted,
                   counters, failure, nexec>>

(* handleExecutionEnd(): a replay that did not reach its target either
   reports with what it has (PR #22) or leaves its state behind; otherwise
   the end-of-execution checks may find a liveness violation (the target is
   the event before the block) or a warning pointing at a thread's last label *)
Incomplete == InReplay /\ buffered /= "none"

EndHandle ==
    /\ phase = "end"
    /\ phase' = "next"
    /\ IF Incomplete THEN
           IF ReportIncompleteReplay
           THEN Report(buffered) /\ UNCHANGED <<pending, detected, warned>>
           ELSE UNCHANGED <<target, buffered, halted, reportedHard, reportedSoft,
                            pending, detected, warned>>
       ELSE
           /\ UNCHANGED <<target, buffered, halted, reportedHard, reportedSoft>>
           /\ \/ UNCHANGED <<pending, detected, warned>>
              \/ /\ ~moot /\ ~halted /\ pending = NoReplay
                 /\ \/ \E t \in Threads:
                          /\ Last(t) = "Blk" /\ glen[t] >= 2
                          /\ pending' = [pos |-> Ev(t, glen[t] - 1), sev |-> "hard"]
                          /\ detected' = detected + 1
                          /\ UNCHANGED warned
                    \/ \E t \in Threads:
                          /\ Last(t) = "Kill"
                          /\ pending' = [pos |-> Ev(t, glen[t]), sev |-> "soft"]
                          /\ warned' = warned + 1
                          /\ UNCHANGED detected
    /\ UNCHANGED <<mode, exec, glen, pos, sched, moot, failure, nexec>>

(* done(): take the queued replay (the graph stays), or go on exploring: a
   rerun from an empty graph under estimation, a revisit that keeps a prefix
   of the graph otherwise, or the end of the exploration *)
Next1 ==
    /\ phase = "next"
    /\ UNCHANGED <<mode, pos, sched, moot, halted, counters, failure, nexec>>
    /\ IF halted THEN
           /\ phase' = "done"
           /\ UNCHANGED <<exec, glen, target, buffered, pending>>
       ELSE IF pending /= NoReplay THEN
           /\ target' = pending.pos /\ buffered' = pending.sev /\ pending' = NoReplay
           /\ phase' = "start"
           /\ UNCHANGED <<exec, glen>>
       ELSE
           /\ UNCHANGED <<target, buffered, pending>>
           /\ \/ /\ phase' = "done"
                 /\ UNCHANGED <<exec, glen>>
              \/ /\ nexec < MaxExecs
                 /\ phase' = "start"
                 /\ \/ /\ mode = "estimate"
                       /\ exec' \in Execs
                       /\ glen' = [t \in Threads |-> 0]
                    \/ /\ mode = "verify"
                       /\ \E cut \in [Threads -> 0..(MaxBody + 1)]:
                            /\ \A t \in Threads: cut[t] <= glen[t]
                            /\ cut /= glen
                            /\ glen' = cut
                            /\ \E e \in Execs:
                                 /\ \A t \in Threads:
                                      /\ Len(e[t]) >= cut[t]
                                      /\ \A i \in 1..cut[t]: e[t][i] = exec[t][i]
                                 /\ exec' = e

Done == phase = "done" /\ UNCHANGED vars

Next == StartExec \/ RunReplay \/ RunPolicy \/ EndExec \/ EndHandle \/ Next1 \/ Done

Spec == Init /\ [][Next]_vars

(* No internal check fails: the replay target is always in the graph the
   schedule is computed from, and a kill label is never re-added outside the
   replay that targets it. *)
NoInternalFailure == failure = "none"

(* Every hard error found is printed (exactly once, as the first one halts). *)
HardErrorsReported == phase = "done" => reportedHard = detected

ReportedAtMostOnce == reportedHard <= 1 /\ reportedSoft <= warned

=============================================================================
