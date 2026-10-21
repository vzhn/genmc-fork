# Supported APIs

GenMC supports verifying programs that use standard concurrency APIs.
Below we list the supported functions for C/C++ and Rust.

## C/C++ APIs

Apart from the C11 API (defined in `stdatomic.h`) and the `assert()`
function used to define safety specifications, GenMC supports the following:

### `pthread` API

The following functions are supported from `pthread.h`:

- `int pthread_create (pthread_t *, const pthread_attr_t *, void *(*) (void *), void *)`
- `int pthread_join (pthread_t, void **)`
- `pthread_t pthread_self (void)`
- `void pthread_exit (void *)`
- `int pthread_mutex_init (pthread_mutex_t *, const pthread_mutexattr_t *)`
- `int pthread_mutex_lock (pthread_mutex_t *)`
- `int pthread_mutex_trylock (pthread_mutex_t *)`
- `int pthread_mutex_unlock (pthread_mutex_t *)`
- `int pthread_mutex_destroy (pthread_mutex_t *)`
- `int pthread_barrier_init (pthread_barrier_t *, const pthread_barrierattr_t *, unsigned)`
- `int pthread_barrier_wait (pthread_barrier_t *)`
- `int pthread_barrier_destroy (pthread_barrier_t *)`
- `int pthread_rwlock_init (pthread_rwlock_t *, const pthread_rwlockattr_t *)`,
  `pthread_rwlock_rdlock`, `pthread_rwlock_tryrdlock`, `pthread_rwlock_wrlock`,
  `pthread_rwlock_trywrlock`, `pthread_rwlock_unlock` and `pthread_rwlock_destroy`
  (`(pthread_rwlock_t *)`); at most `__VERIFIER_MAX_THREAD_NUM` threads (default 8)
- `int pthread_cond_init (pthread_cond_t *, const pthread_condattr_t *)`,
  `pthread_cond_wait (pthread_cond_t *, pthread_mutex_t *)`, `pthread_cond_signal`,
  `pthread_cond_broadcast` and `pthread_cond_destroy` (`(pthread_cond_t *)`)
- `int pthread_once (pthread_once_t *, void (*) (void))`: a caller that finds the
  init routine running waits until it returns
- `int pthread_key_create (pthread_key_t *, void (*) (void *))`,
  `int pthread_key_delete (pthread_key_t)`, `void *pthread_getspecific (pthread_key_t)`
  and `int pthread_setspecific (pthread_key_t, const void *)`: at most
  `__VERIFIER_MAX_KEY_NUM` keys (default 8), never reused; destructors are not called
- `int pthread_atfork (void (*) (void), void (*) (void), void (*) (void))`: at most
  `__VERIFIER_MAX_ATFORK_NUM` registrations (default 8). There is no `fork()`; a
  program runs the handlers with `__VERIFIER_atfork_prepare()` (the prepare
  handlers, last registered first), then `__VERIFIER_atfork_parent()` or
  `__VERIFIER_atfork_child()` (the parent or child handlers, first registered
  first, of those that `__VERIFIER_atfork_prepare()` ran)

The limits can be changed with compiler flags, e.g. `genmc -- -D__VERIFIER_MAX_KEY_NUM=16 file.c`.

### `stdlib` API

The following functions are supported from `stdlib.h`:

- `void abort(void)`
- `int abs(int)`
- `int atoi(const char *)`
- `void free(void *)`
- `void *malloc(size_t)`
- `void *calloc(size_t, size_t)` (the block counts as initialized by the allocation, with no per-byte store events)
- `void *aligned_alloc(size_t, size_t)`

### `stdio`, `unistd` and `fcntl` API

The following functions are supported for I/O:

- `int printf(const char *, ...)`

Note that these functions are not guaranteed to work properly in all scenarios.

### Linux-kernel API

`<lkmm.h>` provides the Linux kernel's concurrency primitives (marked
accesses, barriers, atomics, spinlocks, RCU and SRCU), for programs
verified with `-lkmm`; see [the LKMM section](features.md#the-linux-kernel-memory-model-lkmm)
for the list. (Under the other models it maps them onto C11 atomics, an
approximation; SRCU is `-lkmm` only.)

### SV-COMP API

The following functions from the ones defined in SV-COMP [\[SV-COMP 2019\]](references.md#sv-comp) are supported:

- `void __VERIFIER_assume(int)`
- `int __VERIFIER_nondet_int(void)`

Note that, since GenMC is a stateless model checker,
`__VERIFIER_nondet_int()` only "simulates" data non-determinism, and
does actually provide support for it. More specifically, the sequence
of numbers it produces for each thread remains the same across
different executions.

## Rust APIs

GenMC provides support for a subset of the Rust standard library, primarily focused on concurrency and memory management.

### Threading

- `std::thread::spawn` & `join`
- `std::thread::scope`
- `std::thread::spawn_f` / `std::thread::spawn_f_args`
- `yield_now` (no-op)
- `sleep` (no-op)

*Note:* `spawn_f` and `spawn_f_args` are custom, lightweight, non-std alternatives to the standard library's `spawn`. These can be used in cases where the standard spawn currently fails.

### Synchronization (Mutex)

The full `std::sync::Mutex` tools are provided with the exact same functionality as in Rust’s standard library:

- `Mutex::lock`
- `Mutex::try_lock`
- `Mutex::is_poisoned` / `clear_poisoned`

*Note:* A Mutex in GenMC is never considered poisoned. Unlock is automatically performed on drop.

### Memory Allocation

Heap memory can be allocated and managed using `Box` or `std::alloc`. Reallocation of memory is currently not supported.
