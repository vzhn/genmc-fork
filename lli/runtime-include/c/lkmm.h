/*
 * Linux-kernel primitives for GenMC: the names of the kernel memory model's
 * tools/memory-model/linux-kernel.def, plus a few that kernel code uses,
 * mapped onto C11 atomics.
 *
 * Under -lkmm (GenMC defines __GENMC_LKMM__) the barriers and the fully
 * ordered RMWs are GenMC's own events, which the LKMM checker orders as the
 * kernel's memory model does. Under the other models the header maps them
 * onto C11 atomics; run such programs under -imm, which keeps the
 * dependencies that READ_ONCE()/rcu_dereference() ordering relies on. That
 * mapping is an approximation of the Linux-kernel memory model (LKMM);
 * scripts/lkmm/README.md has the differences measured against herd7. The
 * ones known in advance:
 * - stronger than LKMM: smp_rmb()/smp_wmb() are acquire/release fences
 *   (they also order reads before writes); smp_mb__before_atomic(),
 *   smp_mb__after_atomic(), smp_mb__after_spinlock() and
 *   smp_mb__after_unlock_lock() are full fences wherever they stand; a
 *   fully ordered cmpxchg() is fenced before the CAS even when it fails;
 *   rcu_read_lock() and rcu_read_unlock() carry full fences;
 * - weaker than LKMM: IMM orders a dependency into a write only, so
 *   rcu_dereference(p) followed by READ_ONCE(*p) is unordered; the
 *   cumulativity of smp_mb() and of release chains (LKMM's prop) is
 *   missing; a thread's spin_unlock() followed by its spin_lock() orders
 *   nothing (po-unlock-lock-po).
 * Under the other models RCU is a reader-flag implementation:
 * synchronize_rcu() waits for every reader it sees, its own included, so
 * calling it inside a read-side critical section deadlocks. The flags are
 * indexed by GenMC's thread id: a program with more than LKMM_MAX_THREADS
 * (16) threads must define it higher. Under -lkmm RCU is the kernel model's.
 *
 * SRCU is provided under -lkmm only, as the kernel model has it:
 * srcu_read_lock() reads its srcu_struct and returns the value read,
 * srcu_read_unlock() writes the index it gets back to it, and an unlock
 * matches the lock whose value it writes (srcu_down_read() and
 * srcu_up_read() may be in different threads).
 */

#ifndef __LKMM_H__
#define __LKMM_H__

#ifdef __CLANG_STDATOMIC_H
#error "Only one of <stdatomic.h> and <lkmm.h> may be used!"
#endif

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <genmc.h>

/*
 * Pointer arguments go through __lkmm_ptr(); scripts/lkmm/lkmm-c11.h
 * predefines it (and LKMM_WORD_ATOMIC) for the untyped registers of herd7's
 * C litmus tests
 */
#ifndef __lkmm_ptr
#define __lkmm_ptr(X) (X)
#endif

/*******************************************************************************
 **                        MARKED ACCESSES AND FENCES
 ******************************************************************************/

/* X is an lvalue */
#define READ_ONCE(X) __atomic_load_n(&(X), __ATOMIC_RELAXED)
#define WRITE_ONCE(X, V) __atomic_store_n(&(X), (V), __ATOMIC_RELAXED)

/* X is a pointer */
#define smp_load_acquire(X) __atomic_load_n(__lkmm_ptr(X), __ATOMIC_ACQUIRE)
#define smp_store_release(X, V) __atomic_store_n(__lkmm_ptr(X), (V), __ATOMIC_RELEASE)

/* RCU-protected pointers; X is an lvalue */
#define rcu_dereference(X) READ_ONCE(X)
#define rcu_dereference_raw(X) READ_ONCE(X)
#define rcu_dereference_check(X, C) READ_ONCE(X)
#define rcu_dereference_protected(X, C) READ_ONCE(X)
#define rcu_access_pointer(X) READ_ONCE(X)
#define rcu_assign_pointer(X, V) __atomic_store_n(&(X), (V), __ATOMIC_RELEASE)
#define RCU_INIT_POINTER(X, V) WRITE_ONCE(X, V)

/* Fences; barrier() is a compiler barrier, and GenMC never reorders */
#define barrier() do { } while (0)
#define cpu_relax() do { } while (0)
#ifdef __GENMC_LKMM__
#define smp_mb() __VERIFIER_lkmm_fence(GENMC_LKMM_FENCE_MB)
#define smp_rmb() __VERIFIER_lkmm_fence(GENMC_LKMM_FENCE_RMB)
#define smp_wmb() __VERIFIER_lkmm_fence(GENMC_LKMM_FENCE_WMB)
#define smp_mb__before_atomic() __VERIFIER_lkmm_fence(GENMC_LKMM_FENCE_BEFORE_ATOMIC)
#define smp_mb__after_atomic() __VERIFIER_lkmm_fence(GENMC_LKMM_FENCE_AFTER_ATOMIC)
#define smp_mb__after_spinlock() __VERIFIER_lkmm_fence(GENMC_LKMM_FENCE_AFTER_SPINLOCK)
#define smp_mb__after_unlock_lock() __VERIFIER_lkmm_fence(GENMC_LKMM_FENCE_AFTER_UNLOCK_LOCK)
#else
#define smp_mb() __atomic_thread_fence(__ATOMIC_SEQ_CST)
#define smp_rmb() __atomic_thread_fence(__ATOMIC_ACQUIRE)
#define smp_wmb() __atomic_thread_fence(__ATOMIC_RELEASE)
#define smp_mb__before_atomic() smp_mb()
#define smp_mb__after_atomic() smp_mb()
#define smp_mb__after_spinlock() smp_mb()
#define smp_mb__after_unlock_lock() smp_mb()
#endif
#ifdef __GENMC_LKMM__
#define smp_mb__after_srcu_read_unlock()					\
	__VERIFIER_lkmm_fence(GENMC_LKMM_FENCE_AFTER_SRCU_READ_UNLOCK)
#endif
#define smp_acquire__after_ctrl_dep() smp_rmb()
#define smp_store_mb(X, V) do { WRITE_ONCE(X, V); smp_mb(); } while (0)

/* Wait until COND, which refers to the value read as VAL, holds; X is a pointer */
#define smp_cond_load_relaxed(X, COND) ({				\
	__typeof__(*__lkmm_ptr(X)) VAL;					\
	__VERIFIER_await((VAL = READ_ONCE(*__lkmm_ptr(X)), (COND)));	\
	VAL; })
#define smp_cond_load_acquire(X, COND) ({				\
	__typeof__(*__lkmm_ptr(X)) VAL;					\
	__VERIFIER_await((VAL = smp_load_acquire(X), (COND)));		\
	VAL; })

/*******************************************************************************
 **                        EXCHANGE AND COMPARE-EXCHANGE
 ******************************************************************************/

/* A value-returning RMW without an ordering suffix is fully ordered: under
 * -lkmm its accesses carry the Mb attribute (a cmpxchg() that fails loses
 * it), under the C11 mapping it stands between two full fences */
#ifdef __GENMC_LKMM__
#define __lkmm_full(E) ({							\
	__VERIFIER_annotate_begin(GENMC_ATTR_LKMM_MB);				\
	__auto_type __lkmm_r = (E);						\
	__VERIFIER_annotate_end(GENMC_ATTR_LKMM_MB);				\
	__lkmm_r; })
#else
#define __lkmm_full(E) ({ smp_mb(); __auto_type __lkmm_r = (E); smp_mb(); __lkmm_r; })
#endif

/* X is a pointer */
#define xchg(X, V) __lkmm_full(__atomic_exchange_n(__lkmm_ptr(X), (V), __ATOMIC_RELAXED))
#define xchg_relaxed(X, V) __atomic_exchange_n(__lkmm_ptr(X), (V), __ATOMIC_RELAXED)
#define xchg_acquire(X, V) __atomic_exchange_n(__lkmm_ptr(X), (V), __ATOMIC_ACQUIRE)
#define xchg_release(X, V) __atomic_exchange_n(__lkmm_ptr(X), (V), __ATOMIC_RELEASE)

/* cmpxchg() returns the old value; a failed one has no ordering */
#define __lkmm_cmpxchg(X, V, W, M) ({						\
	__typeof__(*__lkmm_ptr(X)) __lkmm_o = (V);				\
	__atomic_compare_exchange_n(__lkmm_ptr(X), &__lkmm_o, (W), false, (M),	\
				    __ATOMIC_RELAXED);				\
	__lkmm_o; })
#ifdef __GENMC_LKMM__
#define cmpxchg(X, V, W) ({							\
	__typeof__(*__lkmm_ptr(X)) __lkmm_o = (V);				\
	__lkmm_full(__atomic_compare_exchange_n(__lkmm_ptr(X), &__lkmm_o, (W),	\
						false, __ATOMIC_RELAXED,	\
						__ATOMIC_RELAXED));		\
	__lkmm_o; })
#else
#define cmpxchg(X, V, W) ({							\
	__typeof__(*__lkmm_ptr(X)) __lkmm_o = (V);				\
	smp_mb();								\
	if (__atomic_compare_exchange_n(__lkmm_ptr(X), &__lkmm_o, (W), false,	\
					__ATOMIC_RELAXED, __ATOMIC_RELAXED))	\
		smp_mb();							\
	__lkmm_o; })
#endif
#define cmpxchg_relaxed(X, V, W) __lkmm_cmpxchg(X, V, W, __ATOMIC_RELAXED)
#define cmpxchg_acquire(X, V, W) __lkmm_cmpxchg(X, V, W, __ATOMIC_ACQUIRE)
#define cmpxchg_release(X, V, W) __lkmm_cmpxchg(X, V, W, __ATOMIC_RELEASE)

/* try_cmpxchg() returns whether it succeeded; V points to the expected
 * value, which a failure replaces with the value found */
#define __lkmm_try_cmpxchg(X, V, W, M)						\
	__atomic_compare_exchange_n(__lkmm_ptr(X), (V), (W), false, (M), __ATOMIC_RELAXED)
#ifdef __GENMC_LKMM__
#define try_cmpxchg(X, V, W) __lkmm_full(__lkmm_try_cmpxchg(X, V, W, __ATOMIC_RELAXED))
#else
#define try_cmpxchg(X, V, W) ({							\
	smp_mb();								\
	bool __lkmm_ok = __lkmm_try_cmpxchg(X, V, W, __ATOMIC_RELAXED);		\
	if (__lkmm_ok)								\
		smp_mb();							\
	__lkmm_ok; })
#endif
#define try_cmpxchg_relaxed(X, V, W) __lkmm_try_cmpxchg(X, V, W, __ATOMIC_RELAXED)
#define try_cmpxchg_acquire(X, V, W) __lkmm_try_cmpxchg(X, V, W, __ATOMIC_ACQUIRE)
#define try_cmpxchg_release(X, V, W) __lkmm_try_cmpxchg(X, V, W, __ATOMIC_RELEASE)

/*******************************************************************************
 **                              ATOMIC_T
 ******************************************************************************/

#ifdef LKMM_WORD_ATOMIC
/* herd7's atomic_t is a word: atomic_read(X) is READ_ONCE(*X) */
typedef intptr_t atomic_t;
#define ATOMIC_INIT(i) (i)
#define __lkmm_ctr(X) (*__lkmm_ptr(X))
#else
typedef struct {
	int counter;
} atomic_t;
typedef struct {
	int64_t counter;
} atomic64_t;
typedef atomic64_t atomic_long_t;
#define ATOMIC_INIT(i) { (i) }
#define __lkmm_ctr(X) ((X)->counter)
#endif

/* X is an atomic_t pointer (or an atomic64_t/atomic_long_t one) */
#define atomic_read(X) READ_ONCE(__lkmm_ctr(X))
#define atomic_set(X, V) WRITE_ONCE(__lkmm_ctr(X), (V))
#define atomic_read_acquire(X) smp_load_acquire(&__lkmm_ctr(X))
#define atomic_set_release(X, V) smp_store_release(&__lkmm_ctr(X), (V))

/* Non-value-returning RMWs are unordered; GenMC records that they return
 * nothing (smp_rmb() does not order them in LKMM) */
#define __lkmm_noret(X, OP, V) do {						\
	__VERIFIER_annotate_begin(GENMC_KIND_NONVR);				\
	__atomic_##OP(&__lkmm_ctr(X), (V), __ATOMIC_RELAXED);			\
	__VERIFIER_annotate_end(GENMC_KIND_NONVR);				\
} while (0)
#define atomic_add(V, X) __lkmm_noret(X, fetch_add, V)
#define atomic_sub(V, X) __lkmm_noret(X, fetch_sub, V)
#define atomic_and(V, X) __lkmm_noret(X, fetch_and, V)
#define atomic_or(V, X) __lkmm_noret(X, fetch_or, V)
#define atomic_xor(V, X) __lkmm_noret(X, fetch_xor, V)
#define atomic_andnot(V, X) __lkmm_noret(X, fetch_and, ~(V))
#define atomic_inc(X) atomic_add(1, X)
#define atomic_dec(X) atomic_sub(1, X)

/* Value-returning RMWs: OP is fetch_add, add_fetch, ...; M the ordering */
#define __lkmm_ret(X, OP, V, M) __atomic_##OP(&__lkmm_ctr(X), (V), (M))
#define atomic_add_return(V, X) __lkmm_full(__lkmm_ret(X, add_fetch, V, __ATOMIC_RELAXED))
#define atomic_add_return_relaxed(V, X) __lkmm_ret(X, add_fetch, V, __ATOMIC_RELAXED)
#define atomic_add_return_acquire(V, X) __lkmm_ret(X, add_fetch, V, __ATOMIC_ACQUIRE)
#define atomic_add_return_release(V, X) __lkmm_ret(X, add_fetch, V, __ATOMIC_RELEASE)
#define atomic_sub_return(V, X) __lkmm_full(__lkmm_ret(X, sub_fetch, V, __ATOMIC_RELAXED))
#define atomic_sub_return_relaxed(V, X) __lkmm_ret(X, sub_fetch, V, __ATOMIC_RELAXED)
#define atomic_sub_return_acquire(V, X) __lkmm_ret(X, sub_fetch, V, __ATOMIC_ACQUIRE)
#define atomic_sub_return_release(V, X) __lkmm_ret(X, sub_fetch, V, __ATOMIC_RELEASE)
#define atomic_inc_return(X) atomic_add_return(1, X)
#define atomic_inc_return_relaxed(X) atomic_add_return_relaxed(1, X)
#define atomic_inc_return_acquire(X) atomic_add_return_acquire(1, X)
#define atomic_inc_return_release(X) atomic_add_return_release(1, X)
#define atomic_dec_return(X) atomic_sub_return(1, X)
#define atomic_dec_return_relaxed(X) atomic_sub_return_relaxed(1, X)
#define atomic_dec_return_acquire(X) atomic_sub_return_acquire(1, X)
#define atomic_dec_return_release(X) atomic_sub_return_release(1, X)
#define atomic_fetch_add(V, X) __lkmm_full(__lkmm_ret(X, fetch_add, V, __ATOMIC_RELAXED))
#define atomic_fetch_add_relaxed(V, X) __lkmm_ret(X, fetch_add, V, __ATOMIC_RELAXED)
#define atomic_fetch_add_acquire(V, X) __lkmm_ret(X, fetch_add, V, __ATOMIC_ACQUIRE)
#define atomic_fetch_add_release(V, X) __lkmm_ret(X, fetch_add, V, __ATOMIC_RELEASE)
#define atomic_fetch_sub(V, X) __lkmm_full(__lkmm_ret(X, fetch_sub, V, __ATOMIC_RELAXED))
#define atomic_fetch_sub_relaxed(V, X) __lkmm_ret(X, fetch_sub, V, __ATOMIC_RELAXED)
#define atomic_fetch_sub_acquire(V, X) __lkmm_ret(X, fetch_sub, V, __ATOMIC_ACQUIRE)
#define atomic_fetch_sub_release(V, X) __lkmm_ret(X, fetch_sub, V, __ATOMIC_RELEASE)
#define atomic_fetch_inc(X) atomic_fetch_add(1, X)
#define atomic_fetch_inc_relaxed(X) atomic_fetch_add_relaxed(1, X)
#define atomic_fetch_inc_acquire(X) atomic_fetch_add_acquire(1, X)
#define atomic_fetch_inc_release(X) atomic_fetch_add_release(1, X)
#define atomic_fetch_dec(X) atomic_fetch_sub(1, X)
#define atomic_fetch_dec_relaxed(X) atomic_fetch_sub_relaxed(1, X)
#define atomic_fetch_dec_acquire(X) atomic_fetch_sub_acquire(1, X)
#define atomic_fetch_dec_release(X) atomic_fetch_sub_release(1, X)
#define atomic_fetch_and(V, X) __lkmm_full(__lkmm_ret(X, fetch_and, V, __ATOMIC_RELAXED))
#define atomic_fetch_and_relaxed(V, X) __lkmm_ret(X, fetch_and, V, __ATOMIC_RELAXED)
#define atomic_fetch_and_acquire(V, X) __lkmm_ret(X, fetch_and, V, __ATOMIC_ACQUIRE)
#define atomic_fetch_and_release(V, X) __lkmm_ret(X, fetch_and, V, __ATOMIC_RELEASE)
#define atomic_fetch_or(V, X) __lkmm_full(__lkmm_ret(X, fetch_or, V, __ATOMIC_RELAXED))
#define atomic_fetch_or_relaxed(V, X) __lkmm_ret(X, fetch_or, V, __ATOMIC_RELAXED)
#define atomic_fetch_or_acquire(V, X) __lkmm_ret(X, fetch_or, V, __ATOMIC_ACQUIRE)
#define atomic_fetch_or_release(V, X) __lkmm_ret(X, fetch_or, V, __ATOMIC_RELEASE)
#define atomic_fetch_xor(V, X) __lkmm_full(__lkmm_ret(X, fetch_xor, V, __ATOMIC_RELAXED))
#define atomic_fetch_xor_relaxed(V, X) __lkmm_ret(X, fetch_xor, V, __ATOMIC_RELAXED)
#define atomic_fetch_xor_acquire(V, X) __lkmm_ret(X, fetch_xor, V, __ATOMIC_ACQUIRE)
#define atomic_fetch_xor_release(V, X) __lkmm_ret(X, fetch_xor, V, __ATOMIC_RELEASE)
#define atomic_fetch_andnot(V, X) atomic_fetch_and(~(V), X)
#define atomic_fetch_andnot_relaxed(V, X) atomic_fetch_and_relaxed(~(V), X)
#define atomic_fetch_andnot_acquire(V, X) atomic_fetch_and_acquire(~(V), X)
#define atomic_fetch_andnot_release(V, X) atomic_fetch_and_release(~(V), X)
#define atomic_xchg(X, V) xchg(&__lkmm_ctr(X), V)
#define atomic_xchg_relaxed(X, V) xchg_relaxed(&__lkmm_ctr(X), V)
#define atomic_xchg_acquire(X, V) xchg_acquire(&__lkmm_ctr(X), V)
#define atomic_xchg_release(X, V) xchg_release(&__lkmm_ctr(X), V)
#define atomic_cmpxchg(X, V, W) cmpxchg(&__lkmm_ctr(X), V, W)
#define atomic_cmpxchg_relaxed(X, V, W) cmpxchg_relaxed(&__lkmm_ctr(X), V, W)
#define atomic_cmpxchg_acquire(X, V, W) cmpxchg_acquire(&__lkmm_ctr(X), V, W)
#define atomic_cmpxchg_release(X, V, W) cmpxchg_release(&__lkmm_ctr(X), V, W)
#define atomic_try_cmpxchg(X, V, W) try_cmpxchg(&__lkmm_ctr(X), V, W)
#define atomic_try_cmpxchg_relaxed(X, V, W) try_cmpxchg_relaxed(&__lkmm_ctr(X), V, W)
#define atomic_try_cmpxchg_acquire(X, V, W) try_cmpxchg_acquire(&__lkmm_ctr(X), V, W)
#define atomic_try_cmpxchg_release(X, V, W) try_cmpxchg_release(&__lkmm_ctr(X), V, W)
#define atomic_sub_and_test(V, X) (atomic_sub_return(V, X) == 0)
#define atomic_dec_and_test(X) (atomic_dec_return(X) == 0)
#define atomic_inc_and_test(X) (atomic_inc_return(X) == 0)
#define atomic_add_negative(V, X) (atomic_add_return(V, X) < 0)
#define atomic_add_negative_relaxed(V, X) (atomic_add_return_relaxed(V, X) < 0)
#define atomic_add_negative_acquire(V, X) (atomic_add_return_acquire(V, X) < 0)
#define atomic_add_negative_release(V, X) (atomic_add_return_release(V, X) < 0)

/* Adds V unless the value is W; returns the old value (fully ordered when
 * it adds). atomic_add_unless() returns whether it added */
#ifdef __GENMC_LKMM__
#define atomic_fetch_add_unless(X, V, W) ({					\
	__typeof__(__lkmm_ctr(X)) __lkmm_o = atomic_read(X);			\
	while (__lkmm_o != (W) &&						\
	       !__lkmm_full(__atomic_compare_exchange_n(&__lkmm_ctr(X), &__lkmm_o, \
						       __lkmm_o + (V), false,	\
						       __ATOMIC_RELAXED,	\
						       __ATOMIC_RELAXED)))	\
		;								\
	__lkmm_o; })
#else
#define atomic_fetch_add_unless(X, V, W) ({					\
	__typeof__(__lkmm_ctr(X)) __lkmm_o = atomic_read(X);			\
	smp_mb();								\
	while (__lkmm_o != (W) &&						\
	       !__atomic_compare_exchange_n(&__lkmm_ctr(X), &__lkmm_o,		\
					   __lkmm_o + (V), false,		\
					   __ATOMIC_RELAXED, __ATOMIC_RELAXED))	\
		;								\
	if (__lkmm_o != (W))							\
		smp_mb();							\
	__lkmm_o; })
#endif
#define atomic_add_unless(X, V, W) (atomic_fetch_add_unless(X, V, W) != (W))
#define atomic_inc_not_zero(X) atomic_add_unless(X, 1, 0)

/*******************************************************************************
 **                               SPINLOCKS
 ******************************************************************************/

/* X is a pointer; the interrupt and bottom-half variants are plain locks */
typedef pthread_mutex_t spinlock_t;
#define __SPIN_LOCK_UNLOCKED(x) PTHREAD_MUTEX_INITIALIZER
#define DEFINE_SPINLOCK(x) spinlock_t x = __SPIN_LOCK_UNLOCKED(x)
#define spin_lock_init(X) pthread_mutex_init(X, NULL)
#define spin_lock(X) pthread_mutex_lock(X)
#define spin_unlock(X) pthread_mutex_unlock(X)
#define spin_trylock(X) (pthread_mutex_trylock(X) == 0)
#define spin_is_locked(X) (__atomic_load_n(&(X)->__private, __ATOMIC_RELAXED) != 0)
#define spin_lock_bh(X) spin_lock(X)
#define spin_unlock_bh(X) spin_unlock(X)
#define spin_lock_irq(X) spin_lock(X)
#define spin_unlock_irq(X) spin_unlock(X)
#define spin_lock_irqsave(X, flags) do { (void) (flags); spin_lock(X); } while (0)
#define spin_unlock_irqrestore(X, flags) do { (void) (flags); spin_unlock(X); } while (0)

/*******************************************************************************
 **                                  RCU
 ******************************************************************************/

#ifdef __GENMC_LKMM__
/* LKMM's own RCU: read-side critical sections and grace periods are events
 * that the checker orders by the kernel model's rcu-order */
#define rcu_read_lock() __VERIFIER_lkmm_fence(GENMC_LKMM_RCU_LOCK)
#define rcu_read_unlock() __VERIFIER_lkmm_fence(GENMC_LKMM_RCU_UNLOCK)
#define synchronize_rcu() __VERIFIER_lkmm_fence(GENMC_LKMM_SYNC_RCU)
#else
/* A reader announces itself with a full fence after setting its flag and
 * before clearing it; synchronize_rcu() fences and waits for every reader
 * flag it finds set. Each thread uses the slot of its GenMC thread id */
#ifndef LKMM_MAX_THREADS
#define LKMM_MAX_THREADS 16
#endif
static int __lkmm_rcu_nesting[LKMM_MAX_THREADS];
static int __lkmm_rcu_reader[LKMM_MAX_THREADS];
#define __lkmm_tid() ((int) __VERIFIER_thread_self())

#define rcu_read_lock() do {							\
	if (__lkmm_rcu_nesting[__lkmm_tid()]++ == 0) {				\
		WRITE_ONCE(__lkmm_rcu_reader[__lkmm_tid()], 1);			\
		smp_mb();							\
	}									\
} while (0)
#define rcu_read_unlock() do {							\
	if (--__lkmm_rcu_nesting[__lkmm_tid()] == 0) {				\
		smp_mb();							\
		WRITE_ONCE(__lkmm_rcu_reader[__lkmm_tid()], 0);			\
	}									\
} while (0)
#define synchronize_rcu() do {							\
	smp_mb();								\
	for (int __lkmm_i = 0; __lkmm_i < LKMM_MAX_THREADS; __lkmm_i++)		\
		__VERIFIER_await(READ_ONCE(__lkmm_rcu_reader[__lkmm_i]) == 0);	\
	smp_mb();								\
} while (0)
#endif
#define synchronize_rcu_expedited() synchronize_rcu()
#define rcu_read_lock_bh() rcu_read_lock()
#define rcu_read_unlock_bh() rcu_read_unlock()
#define rcu_read_lock_sched() rcu_read_lock()
#define rcu_read_unlock_sched() rcu_read_unlock()

#ifdef __GENMC_LKMM__
/* SRCU; X is a pointer to a struct srcu_struct */
struct srcu_struct {
	int __lkmm_srcu;
};
#define DEFINE_SRCU(x) struct srcu_struct x
#define DEFINE_STATIC_SRCU(x) static struct srcu_struct x
#define init_srcu_struct(X) (WRITE_ONCE((X)->__lkmm_srcu, 0), 0)
#define cleanup_srcu_struct(X) do { (void) (X); } while (0)
#define srcu_read_lock(X) __VERIFIER_srcu_read_lock((void *) (X))
#define srcu_read_unlock(X, Y) __VERIFIER_srcu_read_unlock((void *) (X), (Y))
#define srcu_down_read(X) srcu_read_lock(X)
#define srcu_up_read(X, Y) srcu_read_unlock(X, Y)
#define synchronize_srcu(X) __VERIFIER_synchronize_srcu((void *) (X))
#define synchronize_srcu_expedited(X) synchronize_srcu(X)
#define srcu_dereference(X, S) READ_ONCE(X)
#define srcu_dereference_check(X, S, C) READ_ONCE(X)
#endif

#endif /* __LKMM_H__ */
