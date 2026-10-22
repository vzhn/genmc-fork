#ifndef __GENMC_H__
#define __GENMC_H__

#include <stdbool.h>
#include <genmc_internal.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * printf() replacement that adds a string node to the execution graph
 */
__attribute__((__nothrow__)) extern
void __VERIFIER_output(const char *msg);

/*
 * Blocks the current execution if the argument is false
 */
#define __VERIFIER_assume(cond) __VERIFIER_assume_internal(cond, GENMC_ASSUME_USER)

/*
 * Models a limited amount of non-determinism by returning
 * a pseudo-random sequence of integers. This sequence
 * is always the same per execution for each thread, so
 * only one value per call is explored: GenMC warns when
 * a program uses these functions.
 *
 * The other variants SV-COMP defines return such a value
 * in their type. (__VERIFIER_nondet_u32, _loff_t and
 * _sector_t are supported too, but their types are the
 * program's own, so a program declares them itself.)
 */
int __VERIFIER_nondet_int(void) __attribute__ ((__nothrow__));
bool __VERIFIER_nondet_bool(void) __attribute__ ((__nothrow__));
char __VERIFIER_nondet_char(void) __attribute__ ((__nothrow__));
unsigned char __VERIFIER_nondet_uchar(void) __attribute__ ((__nothrow__));
short __VERIFIER_nondet_short(void) __attribute__ ((__nothrow__));
unsigned short __VERIFIER_nondet_ushort(void) __attribute__ ((__nothrow__));
unsigned int __VERIFIER_nondet_uint(void) __attribute__ ((__nothrow__));
unsigned int __VERIFIER_nondet_unsigned(void) __attribute__ ((__nothrow__));
long __VERIFIER_nondet_long(void) __attribute__ ((__nothrow__));
unsigned long __VERIFIER_nondet_ulong(void) __attribute__ ((__nothrow__));
long long __VERIFIER_nondet_longlong(void) __attribute__ ((__nothrow__));
unsigned long long __VERIFIER_nondet_ulonglong(void) __attribute__ ((__nothrow__));
size_t __VERIFIER_nondet_size_t(void) __attribute__ ((__nothrow__));
#ifdef __SIZEOF_INT128__
__int128 __VERIFIER_nondet_int128(void) __attribute__ ((__nothrow__));
unsigned __int128 __VERIFIER_nondet_uint128(void) __attribute__ ((__nothrow__));
#endif

/*
 * An (opaque) datatype used by GenMC's thread functions,
 * __VERIFIER_spawn* and __VERIFIER_join (see below).
 */
typedef __VERIFIER_thread_t __VERIFIER_thread_t;

/*
 * Spawns a thread with argument ARG running FUNC.
 * Returns the spawned thread's TID.
 */
__attribute__ ((__nothrow__, always_inline)) static inline
__VERIFIER_thread_t __VERIFIER_spawn (void *(*__start_routine) (void *),
				      void *__restrict __arg)
{
	return __VERIFIER_thread_create(NULL, __start_routine, __arg);
}

/*
 * Spawns a thread with argument ARG running FUNC as symmetric to TH.
 * Returns the spawned thread's TID.
 */
__attribute__ ((__nothrow__, always_inline)) static inline
__VERIFIER_thread_t __VERIFIER_spawn_symmetric (void *(*__start_routine) (void *),
						void *__restrict __arg,
						__VERIFIER_thread_t __th)
{
	return __VERIFIER_thread_create_symmetric(NULL, __start_routine, __arg, __th);
}

/*
 * Joins thread TH and returns its result.
 */
__attribute__ ((__nothrow__, always_inline)) static inline
void *__VERIFIER_join (__VERIFIER_thread_t __th)
{
	return __VERIFIER_thread_join(__th);
}

/*
 * Helper function that joins thread TH, which was spawned as symmetric.
 * This function merely ensures that the result of the joined thread is
 * not going to be used.
 */
__attribute__ ((__nothrow__, always_inline)) static inline
void __VERIFIER_join_symmetric (__VERIFIER_thread_t __th)
{
	__VERIFIER_join(__th);
}

/*
 * Marker functions that can be used to mark the
 * beginning and end of spinloops that are not automatically
 * transformed to assume() statements by GenMC.
 *
 * 1) __VERIFIER_loop_begin() marks the beginning of a loop
 * 2) __VERIFIER_spin_start() marks the beginning of an iteration
 * 3) __VERIFIER_spin_end(cond) ends a given loop iteration if
 *   COND does not hold
 *
 * Example usage:
 *
 *     for (__VERIFIER_loop_begin();
 *          __VERIFIER_spin_start(), ..., __VERIFIER_spin_end(...);
 *          ...) {
 *     // no side-effects, preferrably empty
 * }
 *
 * NOTE: These should _not_ be used on top of the
 * automatic spin-assume transformation.
 */
void __VERIFIER_loop_begin(void) __attribute__ ((__nothrow__));
void __VERIFIER_spin_start(void) __attribute__((__nothrow__));
__attribute__((__nothrow__, always_inline)) static inline
void __VERIFIER_spin_end(bool cond)
{
	__VERIFIER_assume_internal(cond, GENMC_ASSUME_SPINLOOP);
}

/*
 * Waits until COND holds.
 *
 * __VERIFIER_assume(cond) prunes: GenMC discards the executions where COND
 * fails. __VERIFIER_await(cond) waits: a thread whose COND fails is blocked,
 * and with -check-liveness GenMC reports a liveness violation when the wait
 * can never finish (every read COND made returns the latest value and no
 * other thread can make progress).
 *
 * COND is evaluated once per execution, as with an assume, and must not have
 * side effects. An await may be used inside loops, e.g. a retry loop that
 * tries to take a resource and waits for it otherwise.
 *
 * Example: __VERIFIER_await(atomic_load(&ready) == 1);
 */
void __VERIFIER_await_start(void) __attribute__((__nothrow__));
#define __VERIFIER_await(cond)						\
do {									\
	__VERIFIER_await_start();					\
	__VERIFIER_assume_internal(cond, GENMC_ASSUME_AWAIT);		\
} while (0)

/*
 * Marker function that denotes that a store is local.
 * Since non-local stores may preclude the spin-assume transformation,
 * this marker serves as a hint to GenMC.
 */
#define __VERIFIER_local_write(s)			\
do {							\
	__VERIFIER_annotate_begin(GENMC_ATTR_LOCAL);	\
	s;						\
	__VERIFIER_annotate_end(GENMC_ATTR_LOCAL);	\
} while (0)

/*
 * Marker function that denotes that a store is final, that is,
 * that the location of the store will not be assigned again.
 */
#define __VERIFIER_final_write(s)			\
do {							\
	__VERIFIER_annotate_begin(GENMC_ATTR_FINAL);	\
	s;						\
	__VERIFIER_annotate_end(GENMC_ATTR_FINAL);	\
} while (0)

/*
 * Helping instructions. They take as argument a CAS instruction that
 * has a "helping" role (e.g., tail advancement in Michael-Scott
 * queue), and can aid in state-space reduction. They do not return
 * any value.
 * See the manual for a lengthier explanation.
 */
#define __VERIFIER_helped_CAS(c)			\
do {							\
	__VERIFIER_annotate_begin(GENMC_KIND_HELPED);	\
	c;						\
	__VERIFIER_annotate_end(GENMC_KIND_HELPED);	\
} while (0)
#define __VERIFIER_helping_CAS(c)			\
do {							\
	__VERIFIER_annotate_begin(GENMC_KIND_HELPING);	\
	c;						\
	__VERIFIER_annotate_end(GENMC_KIND_HELPING);	\
} while (0)


/*
 * Marker instructions. They take as an argument a read and
 * a CAS instruction (respectively), that mark the beginning/end
 * of a large atomic operation (loop). For example:
 *
 * int old, new;
 * do {
 *     old = __VERIFIER_speculative_read(
 *         atomic_load_explicit(&x, ...);
 *     );
 *     new = old * 42;
 * while (!__VERIFIER_confirming_CAS(
 *            atomic_compare_exchange_strong_explicit(&x, &old, new, ...);
 *       ))
 *
 * NOTE: These should _not_ be used on top of the automatic
 * automatic loop marking transformation.
 */
#define __VERIFIER_speculative_read(c)			\
({							\
	__VERIFIER_annotate_begin(GENMC_KIND_SPECUL);	\
	__auto_type __ret = c;				\
	__VERIFIER_annotate_end(GENMC_KIND_SPECUL);	\
	__ret;						\
})
#define __VERIFIER_confirming_read(c)			\
({							\
	__VERIFIER_annotate_begin(GENMC_KIND_CONFIRM);	\
	__auto_type __ret = c;				\
	__VERIFIER_annotate_end(GENMC_KIND_CONFIRM);	\
	__ret;						\
})
#define __VERIFIER_confirming_CAS(c)			\
({							\
	__VERIFIER_annotate_begin(GENMC_KIND_CONFIRM);	\
	__auto_type __ret = c;				\
	__VERIFIER_annotate_end(GENMC_KIND_CONFIRM);	\
	__ret;						\
})

#define __VERIFIER_final_CAS(c)				\
({							\
	__VERIFIER_annotate_begin(GENMC_ATTR_FINAL);	\
	__auto_type __ret = c;				\
	__VERIFIER_annotate_end(GENMC_ATTR_FINAL);	\
	__ret;						\
})


/*
 * A block of code enclosed in __VERIFIER_optional() is a hint to
 * GenMC that the contents of the block are not crucial for verifying
 * safety properties of the program (e.g., sleep-waiting, no-op
 * functions used when spinning, etc). Using __VERIFIER_optional()
 * should lead to a reduced state space.
 *
 * NOTE: The contents of the optional block should _not_ write to
 * memory or modify variables that are live after the block.
 */
#define __VERIFIER_optional(x)				\
do {							\
	if (__VERIFIER_opt_begin()) {			\
		x;					\
		__VERIFIER_assume(0);			\
	}						\
} while (0)


/*
 * An opaque type for hazard pointers.
 * The interface is similar to the one for atomic variables
 * (i.e., all operations on hazard pointers take the address
 * of a hazard pointer object as a parameter, etc)
 */
typedef __VERIFIER_hazptr_t __VERIFIER_hp_t;

/*
 * Allocates a hazard pointer (and links it to the global list of hazard pointers).
 * Returns an (opaque) __VERIFIER_hp_t.
 */
#define __VERIFIER_hp_alloc()						\
	__VERIFIER_hazptr_alloc()					\

/*
 * Protects P using HP; returns the value protected.
 * (HP needs to be the address of a hazard pointer, while
 * p needs to be the address of an (atomic) variable.)
 */
#define __VERIFIER_hp_protect(hp, p)					\
({									\
	void *_p_ = __atomic_load_n((void **) p, __ATOMIC_ACQUIRE);	\
	__VERIFIER_hazptr_protect(hp, _p_);				\
	_p_;								\
})

/*
 * Clears HP's protection
 */
#define __VERIFIER_hp_clear(hp)			\
	__VERIFIER_hazptr_clear(hp);

/*
 * Deallocates HP (and removes it from the global list).
 * (HP needs to be the address of a hazard pointer.)
 */
#define __VERIFIER_hp_free(hp)			\
	__VERIFIER_hazptr_free(hp)

/*
 * Deallocates P after it ensures that it is no longer protected.
 * This is equivalent to adding P to a list of items that will
 * eventually be freed.
 */
#define __VERIFIER_hp_retire(p)			\
	__VERIFIER_hazptr_retire(p)

/*
 * Specifies that D is going to be allocated in persistent storage
 * (i.e., the respective variable is going to be durable).
 */
#define __VERIFIER_persistent_storage(d)		\
	__attribute__ ((section ("__genmc_persist"))) d

/*
 * Dynamically allocates a new variable with durable storage.
 */
void *__VERIFIER_palloc(size_t);

/*
 * The signature of a recovery routine to be specified
 * by the user. This routine will run after each execution,
 * if the checker is run with the respective flags enabled.
 */
void __VERIFIER_recovery_routine(void) __attribute__ ((__nothrow__));

/*
 * All the file opeartions before this barrier will
 * have persisted to memory when the recovery routine runs.
 * Should be used only once.
 */
void __VERIFIER_pbarrier(void) __attribute__ ((__nothrow__));

/* Markers for method boundaries */
extern void __VERIFIER_method_begin(const char *name, int32_t argVal) __attribute__ ((__nothrow__));
extern void __VERIFIER_method_end(const char *name, int32_t retval) __attribute__ ((__nothrow__));

/*
 * Datatype for the partial-lock specification device (Lahav et al. 2023).
 * Should only be used along with the --collect-spec flag
 */
typedef struct __VERIFIER_plock __VERIFIER_plock_t;

/* Lock/unlock utilities for the partial lock */
__attribute__ ((__nothrow__, always_inline)) static inline
int __VERIFIER_plock_lock(__VERIFIER_plock_t *plock)
{
	__VERIFIER_annotate_begin(GENMC_KIND_PLOCK);
	int __res = __VERIFIER_mutex_lock(&plock->lock);
	__VERIFIER_annotate_end(GENMC_KIND_PLOCK);
	return __res;
}

 __attribute__ ((__nothrow__, always_inline ))static inline
int __VERIFIER_plock_unlock(__VERIFIER_plock_t *plock)
{
	__VERIFIER_annotate_begin(GENMC_KIND_PLOCK);
	 int __res = __VERIFIER_mutex_unlock(&plock->lock);
	__VERIFIER_annotate_end(GENMC_KIND_PLOCK);
	return __res;
}

#ifdef __cplusplus
}
#endif

#endif /* __GENMC_H__ */
