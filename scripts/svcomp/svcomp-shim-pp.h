/*
 * This program is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 */
/*
 * SV-COMP compatibility shim for GenMC, for preprocessed tasks: tasks that
 * declare the pthread API (pthread_t as unsigned long, the mutex unions,
 * __assert_fail, ...) themselves instead of including <pthread.h>. GenMC's
 * headers would clash with those declarations (genmc_internal.h includes
 * <stddef.h>, whose 64-bit size_t clashes with the ILP32 tasks' own), so
 * this prelude includes nothing: it declares the GenMC entry points it
 * needs, which GenMC recognizes by name, and uses __atomic builtins.
 * svcomp-run.py appends a tail that defines the pthread functions the task
 * declares, in terms of GenMC's primitives and with the task's own types.
 *
 * Nondeterminism, atomic sections and main are handled in svcomp-shim-core.h.
 */
/* GenMC's entry points, as in lli/runtime-include/c/genmc_internal.h */
typedef long __VERIFIER_thread_t;
typedef struct { int __private; } __VERIFIER_attr_t;
typedef struct { int __private; } __VERIFIER_mutex_t;
typedef long __VERIFIER_mutexattr_t;

extern void __VERIFIER_assert_fail(const char *, const char *, int) __attribute__((__nothrow__));
extern int __VERIFIER_thread_create(const __VERIFIER_attr_t *__attr,
				    void *(*__start_routine)(void *),
				    void *__restrict __arg) __attribute__((__nothrow__));
extern void __VERIFIER_thread_exit(void *__retval)
	__attribute__((__noreturn__)) __attribute__((__nothrow__));
extern void *__VERIFIER_thread_join(__VERIFIER_thread_t __th) __attribute__((__nothrow__));
extern __VERIFIER_thread_t __VERIFIER_thread_self(void) __attribute__((__nothrow__));
extern int __VERIFIER_mutex_init(__VERIFIER_mutex_t *__mutex,
				 const __VERIFIER_mutexattr_t *__mutexattr) __attribute__((__nothrow__));
extern int __VERIFIER_mutex_destroy(__VERIFIER_mutex_t *__mutex) __attribute__((__nothrow__));
extern int __VERIFIER_mutex_trylock(__VERIFIER_mutex_t *__mutex) __attribute__((__nothrow__));
extern int __VERIFIER_mutex_lock(__VERIFIER_mutex_t *__mutex) __attribute__((__nothrow__));
extern int __VERIFIER_mutex_unlock(__VERIFIER_mutex_t *__mutex) __attribute__((__nothrow__));
extern void __VERIFIER_assume_internal(_Bool cond, char type) __attribute__((__nothrow__));
extern void __VERIFIER_await_start(void) __attribute__((__nothrow__));
extern void *__VERIFIER_malloc(unsigned long) __attribute__((__nothrow__));
extern void *__VERIFIER_calloc(unsigned long, unsigned long) __attribute__((__nothrow__));
extern void __VERIFIER_free(void *) __attribute__((__nothrow__));

#define __SVC_ASSUME_USER 0	/* GENMC_ASSUME_USER */
#define __SVC_ASSUME_AWAIT 3	/* GENMC_ASSUME_AWAIT */
#define __VERIFIER_assume(cond) __VERIFIER_assume_internal(cond, __SVC_ASSUME_USER)
#define __VERIFIER_await(cond)						\
do {									\
	__VERIFIER_await_start();					\
	__VERIFIER_assume_internal(cond, __SVC_ASSUME_AWAIT);		\
} while (0)

#include "svcomp-shim-core.h"
