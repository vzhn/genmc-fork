# Tool Features

## Available Memory Models

By default, GenMC verifies programs under RC11. However, apart from
RC11, GenMC also supports other models like SC and IMM. The difference
between these memory models (as far as allowed outcomes are concerned)
can be seen in the following program:

```c
/* file: lb.c */
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <assert.h>

atomic_int x;
atomic_int y;

void *thread_1(void *unused)
{
    int a = atomic_load_explicit(&x, memory_order_relaxed);
    atomic_store_explicit(&y, 1, memory_order_relaxed);
    return NULL;
}

void *thread_2(void *unused)
{
    int b = atomic_load_explicit(&y, memory_order_relaxed);
    atomic_store_explicit(&x, 1, memory_order_relaxed);
    return NULL;
}

int main()
{
    pthread_t t1, t2;

    if (pthread_create(&t1, NULL, thread_1, NULL))
        abort();
    if (pthread_create(&t2, NULL, thread_2, NULL))
        abort();

    return 0;
}
```

Under RC11, an execution where both `a` = 1 and `b` = 1 is forbidden,
whereas such an execution is allowed under IMM. To account for such
behaviors, GenMC tracks dependencies between program instructions thus
leading to a constant overhead when verifying programs under models
like IMM.

### Note on Language Memory Models vs Hardware Memory Models

RC11 is a language-level memory model while IMM is a hardware memory
model. Subsequently, the verification results produced by GenMC for
the two models should be interpreted somewhat differently.

What this means in practice is that, when verifying programs under
RC11, the input file is assumed to be the very source code the user
wrote. A successful verification result under RC11 holds all the way
down to the actual executable, due to the guarantees provided by RC11
[\[Lahav et al. 2017\]](references.md#lahav2017repairing).

On the other hand, when verifying programs under IMM, the input file
is assumed to be the assembly code run by the processor (or, more
precisely, a program in IMM's intermediate language). And while GenMC
allows the input file to be a C file (as in the case of RC11), it
assumes that this C file corresponds to an assembly file that is the
result of the compilation of some program in IMM's language. In other
words, program correctness is not preserved across compilation for IMM
inputs.

## The Linux-Kernel Memory Model (LKMM)

With `-lkmm`, GenMC verifies programs written against the Linux kernel's
concurrency primitives under the kernel's own memory model
[\[Alglave et al. 2018\]](references.md#alglave2018lkmm), the one in the
kernel's `tools/memory-model` (`linux-kernel.cat`, variant `lkmmv2`). Such
programs include `<lkmm.h>` instead of `<stdatomic.h>`:

```c
/* file: rcu-free.c */
#include <stdlib.h>
#include <pthread.h>
#include <lkmm.h>

struct foo {
    int a;
};

struct foo *gp;

void *reader(void *unused)
{
    rcu_read_lock();
    struct foo *p = rcu_dereference(gp);
    if (p)
        (void) READ_ONCE(p->a);
    rcu_read_unlock();
    return NULL;
}

void *updater(void *unused)
{
    struct foo *p = gp;

    WRITE_ONCE(gp, NULL);
    synchronize_rcu();
    free(p);
    return NULL;
}

int main()
{
    pthread_t t1, t2;
    struct foo *p = malloc(sizeof(*p));

    p->a = 1;
    gp = p;
    if (pthread_create(&t1, NULL, reader, NULL))
        abort();
    if (pthread_create(&t2, NULL, updater, NULL))
        abort();

    return 0;
}
```

`genmc -lkmm rcu-free.c` explores 2 executions and reports no error: the
grace period orders the reader's critical section before the free.
Without `synchronize_rcu()`, GenMC reports an access of freed memory.

### Primitives

`<lkmm.h>` provides the names of the kernel model's `linux-kernel.def`
and some that kernel code commonly uses:

- marked accesses: `READ_ONCE()`, `WRITE_ONCE()`, `smp_load_acquire()`,
  `smp_store_release()`, `rcu_dereference()`, `rcu_assign_pointer()`,
  `smp_cond_load_relaxed()`/`smp_cond_load_acquire()` (an await);
- barriers: `smp_mb()`, `smp_rmb()`, `smp_wmb()`,
  `smp_mb__before_atomic()`, `smp_mb__after_atomic()`,
  `smp_mb__after_spinlock()`, `smp_mb__after_unlock_lock()`,
  `smp_store_mb()`; `barrier()` is no event;
- `xchg()`, `cmpxchg()` and the `atomic_t` operations in all their
  orderings (fully ordered, `_relaxed`, `_acquire`, `_release`), the
  non-value-returning ones (`atomic_inc()`, ...), `atomic_add_unless()`,
  `atomic_inc_not_zero()`;
- spinlocks: `spin_lock()`, `spin_unlock()`, `spin_trylock()`,
  `spin_is_locked()` (and the `_bh`, `_irq`, `_irqsave` forms);
- RCU: `rcu_read_lock()`, `rcu_read_unlock()` (nested),
  `synchronize_rcu()`;
- SRCU: `srcu_read_lock()`, `srcu_read_unlock()`, `srcu_down_read()`,
  `srcu_up_read()`, `synchronize_srcu()`,
  `smp_mb__after_srcu_read_unlock()`, `DEFINE_SRCU()`,
  `init_srcu_struct()`.

Accesses outside these primitives are plain (C's non-atomic accesses).
Threads are created and joined with `pthread_create()` and
`pthread_join()`, which the kernel model does not have: GenMC treats a
creation and a join as a release write read by an acquire read.

### What GenMC checks

Under `-lkmm`, GenMC reports:

- assertion failures in the executions the model allows;
- data races as the model defines them (its `data-race` flag), on
  complete executions; `-disable-race-detection` turns this off and keeps
  the racy executions, which the model's plain-coherence still shapes;
- invalid RCU usage: `rcu_read_unlock()` without a matching lock, a
  thread that ends inside a critical section, `synchronize_rcu()` inside
  one, and the misuses of SRCU the model flags (unmatched or multiply
  matched `srcu_read_lock()`/`srcu_read_unlock()`, an unlock of another
  index than its lock returned, `synchronize_srcu()` in an RCU critical
  section or in a critical section of its own `srcu_struct`);
- the memory errors of the other models; a grace period orders a reader's
  accesses before a free that follows it.

### Relation to herd7

The kernel model's reference tool is herd7. On C litmus tests, GenMC's
`-lkmm` explores the executions herd7 allows (its "Positive" and
"Negative" ones, with the same final states and counts), with these
exceptions:

- **Control dependencies.** As in the kernel model, a control dependency
  orders only the events inside the conditional: a write after the
  branches join is not ordered after the read the condition tested (a
  compiler could move it above the `if`), and GenMC finds the bugs of code
  that relies on such an order. GenMC has a few dependencies that herd7,
  whose dependencies are syntactic, does not: a value chosen by the branch
  taken (`p = r ? &a : &b`, or a variable set in a branch) depends on the
  condition; so do the events of a function called through a pointer the
  read chose; a branch that leaves its function (an early `return`)
  orders the rest of the function; and so do, for the exploration, a
  marked write after the `if` to a location a marked write in the branch
  writes, and the events after a later fence, release or lock operation
  when the branch has a marked write or a fence. GenMC can also miss an
  execution with a cycle of control dependencies and reads from plain
  writes in branches, which the model allows (a plain write is in no
  happens-before) but GenMC's exploration cannot build.
- The model's `mixed-accesses` flag is not reported.
- herd7's symbolic values for thin-air cycles have no counterpart: no
  execution of the C program has them.
- Executions with failed `cmpxchg()` reads can be counted differently
  (one corpus test has the same final states with another count).

Dependencies are those of the LLVM-IR GenMC runs, after `clang`'s
optimizations, which may remove some that the source has. (GenMC's own
merging of the branches' common code is off under `-lkmm`: it would move
their events out of the conditional.)

`scripts/lkmm` in GenMC's source tree has the tools that compare GenMC
with herd7 on the litmus-test corpora (`README.md` there has the results).

## Race Detection and Memory Errors

For memory models that define the notion of a race, GenMC will report
executions containing races as erroneous. For instance, under RC11,
the following program is racy, as there is no happens-before between
the write of `x` in the first thread and the non-atomic read of `x` in
the second thread (even though the latter causally depends on the
former).

```c
/* file: race.c */
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <assert.h>

atomic_int x;

void *thread_1(void *unused)
{
    atomic_store_explicit(&x, 1, memory_order_relaxed);
    return NULL;
}

void *thread_2(void *unused)
{
    int a, b;

    a = atomic_load_explicit(&x, memory_order_relaxed);
    if (a == 1)
        b = *((int *) &x);
    return NULL;
}

int main()
{
    pthread_t t1, t2;

    if (pthread_create(&t1, NULL, thread_1, NULL))
        abort();
    if (pthread_create(&t2, NULL, thread_2, NULL))
        abort();

    return 0;
}
```

Additionally, for all memory models, GenMC detects some memory races
like accessing memory that has been already freed, accessing dynamic
memory that has not been allocated, or freeing an already freed chunk
of memory.

Race detection can be completely disabled by means of
`-disable-race-detection`, which may yield better performance for
certain programs.

## Barrier-Aware Model Checking (BAM)

GenMC v0.6 comes with built-in support for `pthread_barrier_t`
functions (see [Supported APIs](api.md)) via BAM [\[Kokologiannakis and
Vafeiadis 2021\]](references.md#kokologiannakis2021bam). As an example
of BAM in action, consider the following program:

```c
/* file: bam.c */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>
#include <assert.h>

#ifndef N
# define N 2
#endif

pthread_barrier_t b;
atomic_int x;

void *thread_n(void *unused)
{
    ++x;
    pthread_barrier_wait(&b);
    assert(x == N);
    return NULL;
}

int main()
{
    pthread_t t[N];

    pthread_barrier_init(&b, NULL, N);
    for (int i = 0u; i < N; i++) {
        if (pthread_create(&t[i], NULL, thread_n, NULL))
            abort();
    }

    return 0;
}
```

Running GenMC on the program above results in the following output:

```sh
Number of complete executions explored: 2
Total wall-clock time: 0.01s
```

As can be seen, GenMC treats `barrier_wait` calls as no-ops, and they
do not lead to any additional explorations. (The two executions
explored correspond to the possible ways in which `x` can be
incremented).

However, if we disable BAM by means of the `-disable-bam` switch, we
get the following output:

```sh
Number of complete executions explored: 4
Number of blocked executions seen: 4
Total wall-clock time: 0.01s
```

Note that while BAM can lead to the exploration of exponentially fewer
executions, it can only be used if the result of the `barrier_wait` is
not used. If it is, then using `-disable-bam` is necessary, as GenMC
currently does not enforce this limitation.

## State-Space Bounding

Under SC, GenMC can bound the state-space exploration using either
preemption bounding [\[Marmanis et
al. 2023a\]](references.md#marmanis2023buster) and round-robin bounding
[\[Marmanis and Vafeiadis
2023b\]](references.md#marmanis2023robin-bound).

For instance, in the following program, running GenMC with `--sc
--bound=0 --bound-type=context` will avoid exploring executions that
have one or more (preemptive) context-switches.

```c
/* file: bound.c */
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_int x;

void * thread_1(void * unused)
{
    atomic_store(&x, 1);
    atomic_store(&x, 2);
    return NULL;
}

void * thread_2(void * unused)
{
    atomic_load(&x);
    return NULL;
}

int main()
{
    pthread_t t1, t2;

    if (pthread_create(&t1, NULL, thread_1, NULL))
        abort();
    if (pthread_create(&t2, NULL, thread_2, NULL))
        abort();

    return 0;
}
```

To guarantee that no execution within the bound is missed, some
executions that exceed the bound might also be explored, and are
reported appropriately:

```sh
Number of complete executions explored: 3 (1 exceeded bound)
```

The default bounding type (`round`), on the other hand, only explores
executions within the given bound. For instance, when running GenMC
with `--sc --bound=0`, only the single SC execution that can be
obtained with zero rounds (i.e., in one go) using a left-to-right
round-robin scheduler will be explored.

```sh
Number of complete executions explored: 1
```

Note that, while the round-robin bound does not explore executions
that exceed the limit, the number of executions grows more rapidly as
the bound increases (compared to context bounding).

## Symmetry Reduction

GenMC also employs an experimental symmetry reduction mechanism
[\[Kokologiannakis et
al. 2024\]](references.md#kokologiannakis2024spore), which is
beneficial to use when threads are running the same code.

For instance, in the following program, GenMC explores only one execution instead of 6.

```c
/* file: sr.c */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>
#include <assert.h>

atomic_int x;

void *thread_n(void *unused)
{
    ++x;
    return NULL;
}

int main()
{
    pthread_t t1, t2, t3;

    if (pthread_create(&t1, NULL, thread_n, NULL))
        abort();
    if (pthread_create(&t2, NULL, thread_n, NULL))
        abort();
    if (pthread_create(&t3, NULL, thread_n, NULL))
        abort();

    return 0;
}
```

In order for symmetry reduction to actually take place, the spawned
threads need to share exactly the same code, have exactly the same
arguments (unless the thread function never reads its argument), and
also there must not be any memory access (at the LLVM-IR level) between
the spawn instructions. Accesses to local
variables whose address never leaves their function are exempt, as no
other thread can see them. Threads spawned in a loop into a local
`pthread_t` array are therefore symmetric too:

```c
    pthread_t t[3];

    for (int i = 0; i < 3; i++)
        pthread_create(&t[i], NULL, thread_n, NULL);
```

Symmetry reduction treats unordered writes (two writes to the same
location that no synchronization orders) as an error. If GenMC inferred
all the symmetries itself, it instead starts verification over without
symmetry reduction; `-disable-sr` avoids the first attempt.

To make GenMC use symmetry reduction, one can use the primitive
`__VERIFIER_spawn_symmetric(fun, tid)` (defined in `genmc.h`), the
last argument of which is the thread identifier of the last
(previously spawned) symmetric predecessor.

## Commuting Blind RMWs

Threads that each apply the same relaxed read-modify-write to a counter
and ignore its result, such as statistics or progress counters, write
the same sequence of values in every order. Without help, GenMC still
explores each of their N! coherence orders, unless symmetry reduction
applies. With `-blind-rmw-reduction` (RC11 only), it explores one. In
the following program, each worker gets its own index, so the workers
are not symmetric. The program has 6 executions by default and 1 with
the flag:

```c
/* file: blind.c */
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>

#define N 3

atomic_int hits;
atomic_int seen[N];

void *worker(void *arg)
{
    atomic_store_explicit(&seen[(long)arg], 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&hits, 1, memory_order_relaxed);
    return NULL;
}

int main()
{
    pthread_t t[N];

    for (long i = 0; i < N; i++)
        pthread_create(&t[i], NULL, worker, (void *)i);
    for (int i = 0; i < N; i++)
        pthread_join(t[i], NULL);
    assert(atomic_load_explicit(&hits, memory_order_relaxed) == N);
    return 0;
}
```

Two RMWs are treated as interchangeable when they access the same
location with the same size, operation and operand, the program never
uses their results, they are relaxed, no release fence precedes them in
their thread, and no SC fence is in their happens-before past.

GenMC keeps the order in which such RMWs are added. That loses no
behavior only while nothing observes their order, so it applies only
while every write to the location is one of these RMWs, and only if no
other read lies in their causal past. If a later write to the location,
or a load of it that happens after one of the RMWs but not another,
could observe their order, GenMC reports `Execution not well-formed for
--blind-rmw-reduction`. Such a program has to be checked without the
flag.

## Reference Exploration

GenMC's exploration (TruSt) accepts a backward revisit, a write
revisiting an earlier read, only when the events it removes form a
*maximal extension*. That check is what makes every execution explored
exactly once and the exploration terminate without a visited set, and
it is where the exploration's optimizations meet: a mistake there, or
in a reduction layered on it, silently loses executions. With
`-exhaustive`, GenMC becomes a slow but simple reference against which
the default exploration can be compared:

- every consistent backward revisit is explored (no maximality or
  duplicate check of any kind);
- termination and deduplication come from a visited set: the graph each
  revisit (backward or forward) produces is hashed canonically, and a
  revisit whose graph was produced before is skipped. Without the
  maximality check TruSt can cycle (`w1` revisits `r`; `w2`, re-added,
  revisits `r` back to the graph where it read from `w2`), and the
  visited set is what stops it. Complete executions are counted once
  per canonical graph too; "Number of complete executions explored" is
  the deduplicated number, and an `Exhaustive:` line gives the raw one;
- every optimization that changes which graphs are explored is off:
  symmetry reduction, in-place revisiting, BAM, the helper and
  confirmation patterns, the blind-RMW reduction, load annotations
  (which prune reads that would block), resume points (every run replays
  its graph) and estimation. A line at the start of the run says so.

The canonical hash lists, per thread, the labels in program order with
their kind, location, size, value, ordering, reads-from source,
position in the coherence order of the location, and the created or
joined thread; threads are named by their creation path (the index of
the create event in the parent, recursively), not by their id, since
ids are assigned in creation order and two runs reaching the same graph
through different schedules may number nested threads differently.

Under `-imm`, whose revisits keep events after holes (dependency
tracking), the reference keeps its graphs well formed and consistent:

- a revisit keeps, with every event it keeps, what that event's value and
  existence depend on (its data, address, control and CAS dependencies,
  an RMW's read for its write part, a read's write, its thread's
  creation); a revisit for which that includes the revisited read, or an
  event the graph does not have, is not taken. No kept write then changes
  when the threads fill the holes;
- the views of the revisited graph's labels are computed anew, and the
  graph is checked as a whole (coherence, RMW atomicity, the other
  axioms); an inconsistent one is not explored. A read or write that
  fills a hole and that the kept events leave no coherent choice ends
  the run;
- a complete execution is counted only if it is consistent when checked
  anew; the `Exhaustive:` line says how many were not;
- kept events are moved to their instruction when a CAS before them
  gains or loses its write part (as under `-lkmm`, with replay keys).

The mode is for small programs: memory grows with the visited graphs,
and the exploration repeats work the maximality check avoids. It runs
single-threaded. `scripts/oracle-diff.sh` compares the default
exploration with it over the test suite, and `scripts/fuzz-c11.py` over
random C11 programs; every disagreement is a bug, in the reference or
in the exploration. Under `-lkmm` the mode runs too (kept events are
relocated as in the default exploration), and agrees with herd7 on the
kernel's litmus tests.

## Checking Liveness

GenMC can also check for liveness violations in programs with
spinloops. Consider the following simple program:

```c
/* file: liveness.c */
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_int x;

void *thread_1(void *unused)
{
    while (!x)
        ;
    return NULL;
}

int main()
{
    pthread_t t1;

    if (pthread_create(&t1, NULL, thread_1, NULL))
        abort();

    return 0;
}
```

Since there are no writes to `x`, the loop in `thread_1` above will
never terminate. Indeed, running GenMC with `-check-liveness` produces
a relevant error report:

```sh
Error detected: Liveness violation!
Event (1, 4) in graph:
<-1, 0> main:
    (0, 0): B
    (0, 1): TC [forks 1] L.19
    (0, 2): E
<0, 1> thread_1:
    (1, 0): B
    (1, 1): LOOP_BEGIN
    (1, 2): SPIN_START
    (1, 3): Rsc (x, 0) [INIT] L.9
    (1, 4): BLOCK [spinloop]

Non-terminating spinloop: thread 1
Number of complete executions explored: 0
Number of blocked executions seen: 1
Total wall-clock time: 0.07s
```

The `-check-liveness` switch will automatically check for liveness
violations in all loops that have been captured by the spin-assume
transformation.

### Waiting with `__VERIFIER_await()`

A wait written as `__VERIFIER_assume(cond)` is never reported, because
`__VERIFIER_assume()` prunes: GenMC simply discards the executions where
`cond` fails. To mark a wait, use `__VERIFIER_await(cond)` (defined in
`genmc.h`). It blocks the thread while `cond` is false, and with
`-check-liveness` GenMC reports a liveness violation when the wait can
never finish:

```c
void *waiter(void *unused)
{
    __VERIFIER_await(atomic_load(&ready) == 1);
    return NULL;
}
```

`cond` is evaluated once per execution and must not have side effects.
An await may be used inside loops, e.g., in a retry loop that tries to
take a resource and waits for it otherwise. In execution graphs it shows
as `AWAIT_START`, and a thread blocked in it as `BLOCK[await]`.

Without `-check-liveness`, GenMC prints a tip after the execution counts
when some executions ended with a thread waiting in a spinloop or in
`__VERIFIER_await()`.

## Checking Linearizability

GenMC implements the Relinche algorithm [\[Golovin et
al. 2025\]](references.md#goloving2025relinche) for checking (bounded)
linearizability. This algorithm requires using GenMC with the "Most
Parallel Client" (MPC). Such a client for queues and stacks is already
provided in GenMC's test suite (e.g.,
`tests/correct/relinche/queue/mpc.c`).

To use the client e.g., for a queue, we have to provide a queue
implementation that defines the following methods: `init_queue()`,
`enqueue()`, `dequeue()` and `clear_queue()`. For instance, an
implementation of a Herlihy-Wing queue can be seen below:

```c
/* file: hw-queue.c */
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>
#include <stdbool.h>

#define MAX_NODES    0xff

typedef struct _queue_t {
    _Atomic(int) tail;
    _Atomic(unsigned int) nodes[MAX_NODES] ;
} queue_t;

void init_queue(queue_t *q, int num_threads)
{
}

void clear_queue(queue_t *q, int num_threads)
{
}

void enqueue(queue_t *q, unsigned int val)
{
    int i = atomic_fetch_add_explicit(&q->tail, 1, release);
    assert(i + 1 < MAX_NODES);
    atomic_store_explicit(&q->nodes[i + 1], val, release);
}

bool dequeue(queue_t *q, unsigned int *ret)
{
    bool success = false;
    while (!success) {
        int tail = atomic_load_explicit(&q->tail, acquire);
        for (int i = 0; i <= tail; ++i) {
            if (atomic_load_explicit(&q->nodes[i], acquire) == 0)
                continue;
#ifdef BUG /* Linearizability bug */
            unsigned int v = atomic_exchange_explicit(&q->nodes[i], 0, acquire);
#else
            unsigned int v = atomic_exchange_explicit(&q->nodes[i], 0, acq_rel);
#endif
            if (v != 0) {
                *ret = v;
                success = true;
                break;
            }
        }
        __VERIFIER_assume(success);
    }
    return *ret;
}
```

To check correctness of the above implementation, we first have to
provide a specification. GenMC can produce such a specification file
from a reference implementation, but we can also use one of the
predefined specification files (e.g.,
`tests/correct/queue/queue_spec_rc11.in`) as follows:

```sh
genmc -rc11 -disable-mm-detector --check-lin-spec=spec.in -- -DRTN=2 -DWTN=2 -include hw-queue.c mpc.c"
```

Doing so with the above implementation and spec file, will check for
linearizability of all clients with two `dequeue` and two `enqueue`
operations. GenMC produces the following output:

```sh
GenMC v0.10.3 (LLVM 16.0.6)
Copyright (C) 2024 MPI-SWS. All rights reserved.

*** Compilation complete.
*** Transformation complete.
Tip: Estimating state-space size. For better performance, you can use --disable-estimation.
*** Estimation complete.
Total executions estimate: 33 (+- 37)
Time to completion estimate: 0.03s
*** Verification complete.
No errors were detected.
Number of complete executions explored: 6
Number of blocked executions seen: 10
Number of checked hints: 1
Relinche time: 0.00s
Total wall-clock time: 0.03s
```
