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
 * declares, in terms of GenMC's primitives and with the task's own types;
 * the helpers below do the part that needs no task type.
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

/* Per-thread state for rwlocks and thread-specific data: a slot per thread,
 * indexed by __VERIFIER_thread_self(), that only its own thread accesses.
 * A thread past the last slot calls a function nothing defines, so GenMC
 * stops with "unknown external function" and the task gets no verdict. */
#ifndef SVC_MAX_THREADS
# define SVC_MAX_THREADS 16
#endif
#define SVC_MAX_KEYS 8

extern void __svc_too_many_threads(void);

static long __svc_thread_slot(long n)
{
	long self = __VERIFIER_thread_self();

	if (self >= n)
		__svc_too_many_threads();
	return self;
}

/* Read-write locks, as in GenMC's pthread.h: the lock's storage is an array
 * of N mutexes, one per thread; a reader takes its own, a writer takes all.
 * __svc_rw_writer says which kind of holder a thread is, for unlock. */
static _Bool __svc_rw_writer[SVC_MAX_THREADS];
/* expanded in the tail, where the task has declared pthread_rwlock_t */
#define SVC_RWLOCK_SLOTS (sizeof(pthread_rwlock_t) / sizeof(__VERIFIER_mutex_t))

static int __svc_rw_rdlock(__VERIFIER_mutex_t *ms, long n)
{
	long self = __svc_thread_slot(n < SVC_MAX_THREADS ? n : SVC_MAX_THREADS);

	__svc_rw_writer[self] = 0;
	return __VERIFIER_mutex_lock(&ms[self]);
}

static int __svc_rw_tryrdlock(__VERIFIER_mutex_t *ms, long n)
{
	long self = __svc_thread_slot(n < SVC_MAX_THREADS ? n : SVC_MAX_THREADS);

	__svc_rw_writer[self] = 0;
	return __VERIFIER_mutex_trylock(&ms[self]);
}

static int __svc_rw_wrlock(__VERIFIER_mutex_t *ms, long n)
{
	long self = __svc_thread_slot(n < SVC_MAX_THREADS ? n : SVC_MAX_THREADS);

	for (long i = 0; i < n; i++)
		__VERIFIER_mutex_lock(&ms[i]);
	__svc_rw_writer[self] = 1;
	return 0;
}

static int __svc_rw_trywrlock(__VERIFIER_mutex_t *ms, long n)
{
	long self = __svc_thread_slot(n < SVC_MAX_THREADS ? n : SVC_MAX_THREADS);

	for (long i = 0; i < n; i++) {
		if (__VERIFIER_mutex_trylock(&ms[i]) != 0) {
			while (i-- > 0)
				__VERIFIER_mutex_unlock(&ms[i]);
			return 16; /* EBUSY */
		}
	}
	__svc_rw_writer[self] = 1;
	return 0;
}

static int __svc_rw_unlock(__VERIFIER_mutex_t *ms, long n)
{
	long self = __svc_thread_slot(n < SVC_MAX_THREADS ? n : SVC_MAX_THREADS);

	if (!__svc_rw_writer[self])
		return __VERIFIER_mutex_unlock(&ms[self]);
	for (long i = 0; i < n; i++)
		__VERIFIER_mutex_unlock(&ms[i]);
	__svc_rw_writer[self] = 0;
	return 0;
}

/* Thread-specific data, as in GenMC's pthread.h: a row per thread and a
 * column per key; keys are handed out in order and never reused, and
 * destructors are not called. */
static unsigned int __svc_key_next;
static const void *__svc_key_values[SVC_MAX_THREADS][SVC_MAX_KEYS];

static int __svc_key_create(unsigned int *key)
{
	unsigned int k = __atomic_fetch_add(&__svc_key_next, 1, __ATOMIC_RELAXED);

	if (k >= SVC_MAX_KEYS)
		return 11; /* EAGAIN */
	*key = k;
	return 0;
}

static void *__svc_getspecific(unsigned int key)
{
	long self = __svc_thread_slot(SVC_MAX_THREADS);

	return key < SVC_MAX_KEYS ? (void *) __svc_key_values[self][key] : 0;
}

static int __svc_setspecific(unsigned int key, const void *p)
{
	long self = __svc_thread_slot(SVC_MAX_THREADS);

	if (key >= SVC_MAX_KEYS)
		return 22; /* EINVAL */
	__svc_key_values[self][key] = p;
	return 0;
}

#if SVC_RECURSIVE_MUTEX
/* Recursive mutexes, as in svcomp-shim.h: pthread_mutex_init registers a
 * mutex whose attribute says PTHREAD_MUTEX_RECURSIVE, which then keeps an
 * owner (thread id plus one) and a count around GenMC's mutex. */
# define SVC_MAX_RECURSIVE 4
# define SVC_MUTEX_RECURSIVE 1	/* glibc's PTHREAD_MUTEX_RECURSIVE */

static struct {
	void *m;
	long owner;
	int count;
} __svc_rec[SVC_MAX_RECURSIVE];
static int __svc_nrec;

static int __svc_rec_find(void *m)
{
	int n = __atomic_load_n(&__svc_nrec, __ATOMIC_ACQUIRE);

	for (int i = 0; i < n; i++)
		if (__svc_rec[i].m == m)
			return i;
	return -1;
}

static void __svc_rec_register(void *m, int kind)
{
	if (kind != SVC_MUTEX_RECURSIVE || __svc_rec_find(m) >= 0)
		return;
	int n = __atomic_load_n(&__svc_nrec, __ATOMIC_RELAXED);

	__VERIFIER_assume(n < SVC_MAX_RECURSIVE);
	__svc_rec[n].m = m;
	__atomic_store_n(&__svc_nrec, n + 1, __ATOMIC_RELEASE);
}

static int __svc_mutex_lock(__VERIFIER_mutex_t *m)
{
	int r = __svc_rec_find(m);
	long self = __VERIFIER_thread_self() + 1;

	if (r < 0)
		return __VERIFIER_mutex_lock(m);
	if (__atomic_load_n(&__svc_rec[r].owner, __ATOMIC_RELAXED) == self) {
		__svc_rec[r].count++;
		return 0;
	}
	__VERIFIER_mutex_lock(m);
	__atomic_store_n(&__svc_rec[r].owner, self, __ATOMIC_RELAXED);
	__svc_rec[r].count = 1;
	return 0;
}

static int __svc_mutex_trylock(__VERIFIER_mutex_t *m)
{
	int r = __svc_rec_find(m);
	long self = __VERIFIER_thread_self() + 1;

	if (r < 0)
		return __VERIFIER_mutex_trylock(m);
	if (__atomic_load_n(&__svc_rec[r].owner, __ATOMIC_RELAXED) == self) {
		__svc_rec[r].count++;
		return 0;
	}
	if (__VERIFIER_mutex_trylock(m) != 0)
		return 16; /* EBUSY */
	__atomic_store_n(&__svc_rec[r].owner, self, __ATOMIC_RELAXED);
	__svc_rec[r].count = 1;
	return 0;
}

static int __svc_mutex_unlock(__VERIFIER_mutex_t *m)
{
	int r = __svc_rec_find(m);

	if (r < 0)
		return __VERIFIER_mutex_unlock(m);
	if (--__svc_rec[r].count > 0)
		return 0;
	__atomic_store_n(&__svc_rec[r].owner, 0, __ATOMIC_RELAXED);
	return __VERIFIER_mutex_unlock(m);
}
#else
# define __svc_rec_register(m, kind) ((void) 0)
# define __svc_mutex_lock __VERIFIER_mutex_lock
# define __svc_mutex_trylock __VERIFIER_mutex_trylock
# define __svc_mutex_unlock __VERIFIER_mutex_unlock
#endif

#include "svcomp-shim-core.h"
