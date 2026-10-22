# Command-line Options

A full list of the available command-line options can by viewed by issuing `genmc -help`. Below we describe the ones that are most useful when verifying user programs.

- **`-sc`**: Perform the exploration under the SC memory model
- **`-tso`**: Perform the exploration under the TSO memory model
- **`-ra`**: Perform the exploration under the RA memory model
- **`-rc11`**: Perform the exploration under the RC11 memory model (default)
- **`-imm`**: Perform the exploration under the IMM memory model
- **`-lkmm`**: Perform the exploration under the Linux-kernel memory model (programs using `<lkmm.h>`; see [the LKMM section](features.md#the-linux-kernel-memory-model-lkmm))
- **`-nthreads=<N>`**: Perform verification concurrently (using `N` threads)
- **`-mode=<verify|random|estimate>`**: Choose between exhaustive exploration (default), randomized exploration, and state-space size estimation
- **`-random-budget=<N>`**: Number of executions to sample in `-mode=random`
- **`-cache-instructions`**: Caches instructions to help execution time (sacrifices memory)
- **`-disable-bam`**: Disables Barrier-Aware Model-checking (BAM)
- **`-blind-rmw-reduction`**: Explores one order of identical relaxed RMWs whose results are unused (RC11 only; see [Commuting Blind RMWs](features.md#commuting-blind-rmws))
- **`-exhaustive`**: Reference exploration for small programs: every consistent backward revisit is taken, graphs are deduplicated by a canonical hash, and every reduction is off (see [Reference Exploration](features.md#reference-exploration)); the default under `-lkmm`
- **`-disable-lkmm-exhaustive`**: Under `-lkmm`, explore with the optimal exploration instead: faster and parallel, but it can miss executions whose events after a branch are ordered by events of the branch (see [the LKMM section](features.md#the-linux-kernel-memory-model-lkmm))
- **`-check-liveness`**: Check for liveness violations in spinloops
- **`-unroll=<N>`**: All loops will be executed at most `N` times.
- **`-dump-error-graph=<file>`**: Outputs an erroneous graph to file `<file>`.
- **`-print-error-trace`**: Outputs a sequence of source-code instructions that lead to an error.
- **`-disable-race-detection`**: Disables race detection; racy executions are then explored too (see [Race Detection and Memory Errors](features.md#race-detection-and-memory-errors)).
- **`-program-entry-function=<fun_name>`**: Uses function `<fun_name>` as the program's entry point, instead of `main()`.
- **`-disable-spin-assume`**: Disables the transformation of spin loops to `assume()` statements.
