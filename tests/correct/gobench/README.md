# GoBench kernels

Go concurrency bugs from GoBench's GoKer kernels (Yuan et al., "GoBench: A
Benchmark Suite of Real-World Go Concurrency Bugs", CGO 2021), ported to C on
top of `tests/lib/go.h`, a model of the Go runtime: goroutines, channels and
`select`, `sync.Mutex`, `sync.RWMutex`, `sync.WaitGroup`, `sync.Cond`,
`sync.Once`, timers and context cancellation.

Each directory is one kernel, `<project><pull request>`. The default build is
the program as fixed upstream; a `BUG_*` switch brings the bug back, and the
matching test in `tests/wrong/` checks that GenMC finds it:

- blocking bugs (deadlocks and leaked goroutines) are liveness violations
  under `-check-liveness`, in `tests/wrong/liveness/gobench-*`;
- non-blocking bugs are non-atomic races, in `tests/wrong/racy/gobench-*`,
  or, when they end in a panic (a nil pointer, a send on a closed channel),
  failed assertions, in `tests/wrong/safety/gobench-*`.

Each model's comment names the upstream fix it follows. The kernels are
re-implementations of the bug patterns, not copies of GoBench's Go code.

The correct tests run under RC11 only. The model uses locks and sequentially
consistent atomics, as Go's runtime and `sync/atomic` do, so IMM explores the
same behaviours, more slowly. The wrong liveness tests also run under IMM, and
so does `cockroach10790`, where GenMC once gave two stack variables the same
address under IMM (vzhn/genmc-fork#61).
