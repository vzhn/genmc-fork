# Building GenMC

## Build options

 - `GENMC_DEBUG`: this CMake option will enable diagnostics for GenMC (enabled by default for `Debug` builds)
 - `GENMC_WERROR`: treat compiler warnings as errors for GenMC's own code (`genmc/` and `passes/`); default: OFF, enabled in CI
 - `ENABLE_COVERAGE`: instruments the build for coverage (default: OFF); with clang, adds the `coverage` and `coverage-full` targets (see "Coverage" below)
 - `BUILD_DOCS`: builds documentation (default: OFF)
 - `SANITIZE`: comma-separated sanitizers to enable, e.g. `address,undefined` or `thread` (default: empty)
 - `GENMC_TCMALLOC`: link `genmc` against tcmalloc (faster allocator): `AUTO` (default, if found) / `ON` (required) / `OFF`. Needs `libgoogle-perftools-dev`; disabled under `SANITIZE`. Pin `ON`/`OFF` for reproducible builds.
 - `GENMC_CCACHE`: compile through [ccache](https://ccache.dev): `AUTO` (default, if found) / `ON` (required) / `OFF`. A launcher set with `CMAKE_CXX_COMPILER_LAUNCHER` takes precedence. Rebuilding code compiled before in the same build directory (after a rebase, or switching branches) then takes seconds instead of minutes. To share the cache between build directories and worktrees, let ccache rewrite absolute paths (`ccache -o base_dir=$HOME -o hash_dir=false`); with debug info, objects taken from the cache then name the directory they were first compiled in. A few build directories need a cache of about 20 GB (`ccache -M 20G`).

# Testing GenMC

## Running tests

Run some quick tests:
```shell
./scripts/fast-driver.sh
```

`fast-driver.sh` runs the tests of `scripts/driver.sh` in parallel (`scripts/parallel-driver.sh`): one job per model and test category, `JOBS` at a time (default: the number of cores). Its table has a row per kind of test (correct or wrong), model and category; a failing job's log follows the table. `FAST_DRIVER_SERIAL=1`, or setting `CORRECT_DIRS` or `WRONG_DIRS`, runs it as one `driver.sh`, as before.

Environment variables to affect the testing scripts:
```shell
  GenMC="path/to/genmc/executable"  # Select the GenMC executable to use for the test (default: `${SCRIPT_DIR}/../RelWithDebInfo/bin/genmc`)
  GENMCFLAGS="--flag --another"     # Pass extra flags to any GenMC invocation.
  GENMC_MEMORY_LIMIT=16000000       # Virtual memory per process, in KiB, or "unlimited" (default: 16000000; sanitizer builds: unlimited)
  JOBS=8                            # Jobs at a time for fast-driver.sh (default: the number of cores)
```

`scripts/driver.sh` runs everything under that limit, so that a test that runs away (for example, a `tests/wrong` test with an unbounded loop under a model that lacks its bounding arguments) fails out of memory instead of exhausting the machine.

Example:
```shell
GenMC="Release/bin/genmc" GENMCFLAGS="--disable-estimation" ./scripts/fast-driver.sh
```

## Building with sanitizers

To build with AddressSanitizer + UBSan:
```shell
cmake -DGENMC_DEBUG=ON -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DSANITIZE=address,undefined \
      -DCMAKE_PREFIX_PATH=/usr/lib/llvm-19/cmake \
      -B Asan -S .
cmake --build Asan -j$(nproc)
ASAN_OPTIONS=detect_leaks=0 ./Asan/bin/unit_tests
GenMC=./Asan/bin/genmc ASAN_OPTIONS=detect_leaks=0 ./scripts/fast-driver.sh
```

`ASAN_OPTIONS=detect_leaks=0` suppresses leak reports from LLVM's arena allocators, which
intentionally do not free on exit. For ThreadSanitizer use `SANITIZE=thread`.
`address` and `thread` are mutually exclusive.

## Coverage

With clang, `ENABLE_COVERAGE=ON` builds with source-based coverage and adds two targets:

```bash
cmake -S . -B Cov -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_COMPILER=clang++ \
      -DCMAKE_C_COMPILER=clang -DBUILD_TESTS=ON -DENABLE_COVERAGE=ON
cmake --build Cov --target coverage        # unit_tests and driver_tests (seconds)
cmake --build Cov --target coverage-full   # also scripts/driver.sh --fast (minutes)
```

Both write `Cov/coverage/`: `html/index.html` (annotated sources), `report.txt` (per file), `summary.txt` (per directory, one row per layer and group of `genmc/genmc`, plus `lli` and `passes`), and `coverage.lcov`. `llvm-profdata` and `llvm-cov` are taken from the compiler's directory, so that they read its profile format. Test sources, the build directory and system headers are left out. The regression suite runs under `driver.sh`'s memory limit (`GENMC_MEMORY_LIMIT`). `llvm-cov` warns about functions with "mismatched data": inline functions compiled into several binaries with different settings (`driver_tests` has no LLVM). The warning is harmless.

With GCC, `ENABLE_COVERAGE=ON` adds `--coverage` (gcov) and no report targets.

# Modifying GenMC

## Pull requests

Please submit PRs as series of atomic, easily reversible commits.
Code must be formatted with `clang-format` and free of `clang-tidy` warnings (`scripts/lint.sh`).

## Code conventions

### Comments

Please use `/*` and `*/` as comment delimiters.

Initially, these were used to separate our modifications from the
original code in LLVM code (which uses `//`), but now it's just a
convention.

### Classes

- Declaration order within a class (public -> protected -> private):
  1. Types and type aliases
  2. Static constants/functions (e.g., factories)
  3. Ctors, assignment operators and dtor
  4. All other functions (ending with operators and friends)
  5. Data members (static -> nonstatic)

### Naming

- CamelCase for everything apart from things mimicking STL
  functionality (LLVM utilities remain unmodified)
- Names start with a lowercase letter
- Member variables should end with an underscore (_)

### Error and assertion macros

`genmc/genmc/Support/Error.hpp` provides three macros for invariant checking.
Their behavior can be tuned with `GENMC_DEBUG`.

| Macro | Debug | Release |
|-------|-------|---------|
| `VERIFY(cond[, msg])` | Crashes with message | Crashes with message |
| `ASSERT(cond[, msg])` | Crashes with message | No-op |
| `UNREACHABLE([msg])` | Crashes with message | `__builtin_trap()` (SIGILL) |

## Adding new label types

To add a new label, follow these steps:

1. Add a number and a name for the new label in `genmc/genmc/Execution/EventLabel.def`
2. Add a class for the new label in `genmc/genmc/Execution/EventLabel.hpp`.
   Macros are provided for dummy labels and standard subclasses
3. Add a case for the new label in `genmc/genmc/Execution/LabelVisitor.hpp`
4. Define how the label should be printed in `genmc/genmc/Execution/EventLabel.cpp`
   (optionally: LabelPrinterBase too)
5. Create a handler for the new label in `genmc/genmc/Verification/Core/GenMCDriver.{hpp,cpp}`
   (if necessary), and at DriverHandlerDispatcher
6. In case a new LLVM-IR function leads to the creation of
   the new label:
   - Add a number and a name for the internal function in
   `passes/passes/InternalFunction.def`.
   - Define a function that dispatches the driver at
   `lli/Runtime/Execution.cpp`

*Note:* the procedure above describes the bare minimum, and works well
for dummy labels or subclasses of existing labels. If e.g., the new
label has attributes like location then extra changes might be
required to ensure that iterators, etc still work.
