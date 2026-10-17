/*
 * Linux-kernel primitives mapped onto C11 atomics, for running kernel-style
 * code and C litmus tests under GenMC's existing models (use -imm: it keeps
 * dependencies, which READ_ONCE()/rcu_dereference() ordering relies on).
 *
 * This is an approximation of the Linux-kernel memory model (LKMM), not LKMM.
 * The differences that are known in advance all make the mapping stronger,
 * so it can miss behaviours that LKMM allows:
 * - smp_rmb()/smp_wmb() are acquire/release fences, which also order
 *   reads before writes (and smp_wmb() reads before writes);
 * - smp_mb__{before,after}_atomic(), smp_mb__after_spinlock() and
 *   smp_mb__after_unlock_lock() are full fences, not tied to an RMW or lock;
 * - a fully ordered cmpxchg() is fenced before the CAS even when it fails;
 * - RCU is a reader-flag implementation (readers publish themselves with a
 *   full fence; synchronize_rcu() waits for every reader it sees), not LKMM's
 *   grace-period axiom.
 * Others (cumulativity, the po-unlock-lock-po ordering, plain accesses) are
 * what the differential runs against herd7 measure.
 *
 * Code that uses RCU must define __LK_TID (0 <= __LK_TID < LK_MAX_THREADS) to
 * the calling thread's index; scripts/lkmm/litmus2c.py does so per thread.
 *
 * Not provided: SRCU, smp_cond_load_*(), atomic_add_unless(),
 * spin_is_locked().
 */
#ifndef LKMM_C11_H
#define LKMM_C11_H

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <genmc.h>

/* Marked accesses; X is an lvalue */
#define READ_ONCE(X) __atomic_load_n(&(X), __ATOMIC_RELAXED)
#define WRITE_ONCE(X, V) __atomic_store_n(&(X), (V), __ATOMIC_RELAXED)
#define rcu_dereference(X) READ_ONCE(X)
#define rcu_assign_pointer(X, V) __atomic_store_n(&(X), (V), __ATOMIC_RELEASE)

/* Release/acquire; X is a pointer */
#define smp_load_acquire(X) __atomic_load_n((X), __ATOMIC_ACQUIRE)
#define smp_store_release(X, V) __atomic_store_n((X), (V), __ATOMIC_RELEASE)

/* Fences */
#define barrier() do {} while (0)
#define smp_mb() __atomic_thread_fence(__ATOMIC_SEQ_CST)
#define smp_rmb() __atomic_thread_fence(__ATOMIC_ACQUIRE)
#define smp_wmb() __atomic_thread_fence(__ATOMIC_RELEASE)
#define smp_mb__before_atomic() smp_mb()
#define smp_mb__after_atomic() smp_mb()
#define smp_mb__after_spinlock() smp_mb()
#define smp_mb__after_unlock_lock() smp_mb()
#define smp_store_mb(X, V) do { WRITE_ONCE(X, V); smp_mb(); } while (0)

/* A value-returning RMW without an ordering suffix is fully ordered */
#define __LK_FULL(E) ({ smp_mb(); __auto_type __lk_r = (E); smp_mb(); __lk_r; })

/* Exchange; X is a pointer */
#define xchg(X, V) __LK_FULL(__atomic_exchange_n((X), (V), __ATOMIC_RELAXED))
#define xchg_relaxed(X, V) __atomic_exchange_n((X), (V), __ATOMIC_RELAXED)
#define xchg_acquire(X, V) __atomic_exchange_n((X), (V), __ATOMIC_ACQUIRE)
#define xchg_release(X, V) __atomic_exchange_n((X), (V), __ATOMIC_RELEASE)

/* Compare-and-exchange returns the old value; a failed one has no ordering */
#define __lk_cmpxchg(X, V, W, S) ({						\
	__auto_type __lk_o = (V);						\
	__atomic_compare_exchange_n((X), &__lk_o, (W), false, (S), __ATOMIC_RELAXED); \
	__lk_o; })
#define cmpxchg(X, V, W) ({							\
	__auto_type __lk_o = (V);						\
	smp_mb();								\
	if (__atomic_compare_exchange_n((X), &__lk_o, (W), false,		\
					__ATOMIC_RELAXED, __ATOMIC_RELAXED))	\
		smp_mb();							\
	__lk_o; })
#define cmpxchg_relaxed(X, V, W) __lk_cmpxchg(X, V, W, __ATOMIC_RELAXED)
#define cmpxchg_acquire(X, V, W) __lk_cmpxchg(X, V, W, __ATOMIC_ACQUIRE)
#define cmpxchg_release(X, V, W) __lk_cmpxchg(X, V, W, __ATOMIC_RELEASE)

/* atomic_t; X is an atomic_t pointer */
typedef struct { int counter; } atomic_t;
#define ATOMIC_INIT(i) { (i) }
#define atomic_read(X) READ_ONCE((X)->counter)
#define atomic_set(X, V) WRITE_ONCE((X)->counter, (V))
#define atomic_read_acquire(X) smp_load_acquire(&(X)->counter)
#define atomic_set_release(X, V) smp_store_release(&(X)->counter, (V))

/* Non-value-returning RMWs are unordered */
#define __lk_op(X, OP, V) ((void) __atomic_##OP(&(X)->counter, (V), __ATOMIC_RELAXED))
#define atomic_add(V, X) __lk_op(X, fetch_add, V)
#define atomic_sub(V, X) __lk_op(X, fetch_sub, V)
#define atomic_and(V, X) __lk_op(X, fetch_and, V)
#define atomic_or(V, X) __lk_op(X, fetch_or, V)
#define atomic_xor(V, X) __lk_op(X, fetch_xor, V)
#define atomic_andnot(V, X) __lk_op(X, fetch_and, ~(V))
#define atomic_inc(X) atomic_add(1, X)
#define atomic_dec(X) atomic_sub(1, X)

/* Value-returning RMWs: OP is fetch_add, add_fetch, ...; M the ordering */
#define __lk_ret(X, OP, V, M) __atomic_##OP(&(X)->counter, (V), (M))
#define atomic_add_return(V, X) __LK_FULL(__lk_ret(X, add_fetch, V, __ATOMIC_RELAXED))
#define atomic_add_return_relaxed(V, X) __lk_ret(X, add_fetch, V, __ATOMIC_RELAXED)
#define atomic_add_return_acquire(V, X) __lk_ret(X, add_fetch, V, __ATOMIC_ACQUIRE)
#define atomic_add_return_release(V, X) __lk_ret(X, add_fetch, V, __ATOMIC_RELEASE)
#define atomic_sub_return(V, X) __LK_FULL(__lk_ret(X, sub_fetch, V, __ATOMIC_RELAXED))
#define atomic_sub_return_relaxed(V, X) __lk_ret(X, sub_fetch, V, __ATOMIC_RELAXED)
#define atomic_sub_return_acquire(V, X) __lk_ret(X, sub_fetch, V, __ATOMIC_ACQUIRE)
#define atomic_sub_return_release(V, X) __lk_ret(X, sub_fetch, V, __ATOMIC_RELEASE)
#define atomic_inc_return(X) atomic_add_return(1, X)
#define atomic_inc_return_relaxed(X) atomic_add_return_relaxed(1, X)
#define atomic_inc_return_acquire(X) atomic_add_return_acquire(1, X)
#define atomic_inc_return_release(X) atomic_add_return_release(1, X)
#define atomic_dec_return(X) atomic_sub_return(1, X)
#define atomic_dec_return_relaxed(X) atomic_sub_return_relaxed(1, X)
#define atomic_dec_return_acquire(X) atomic_sub_return_acquire(1, X)
#define atomic_dec_return_release(X) atomic_sub_return_release(1, X)
#define atomic_fetch_add(V, X) __LK_FULL(__lk_ret(X, fetch_add, V, __ATOMIC_RELAXED))
#define atomic_fetch_add_relaxed(V, X) __lk_ret(X, fetch_add, V, __ATOMIC_RELAXED)
#define atomic_fetch_add_acquire(V, X) __lk_ret(X, fetch_add, V, __ATOMIC_ACQUIRE)
#define atomic_fetch_add_release(V, X) __lk_ret(X, fetch_add, V, __ATOMIC_RELEASE)
#define atomic_fetch_sub(V, X) __LK_FULL(__lk_ret(X, fetch_sub, V, __ATOMIC_RELAXED))
#define atomic_fetch_sub_relaxed(V, X) __lk_ret(X, fetch_sub, V, __ATOMIC_RELAXED)
#define atomic_fetch_sub_acquire(V, X) __lk_ret(X, fetch_sub, V, __ATOMIC_ACQUIRE)
#define atomic_fetch_sub_release(V, X) __lk_ret(X, fetch_sub, V, __ATOMIC_RELEASE)
#define atomic_fetch_inc(X) atomic_fetch_add(1, X)
#define atomic_fetch_inc_relaxed(X) atomic_fetch_add_relaxed(1, X)
#define atomic_fetch_inc_acquire(X) atomic_fetch_add_acquire(1, X)
#define atomic_fetch_inc_release(X) atomic_fetch_add_release(1, X)
#define atomic_fetch_dec(X) atomic_fetch_sub(1, X)
#define atomic_fetch_dec_relaxed(X) atomic_fetch_sub_relaxed(1, X)
#define atomic_fetch_dec_acquire(X) atomic_fetch_sub_acquire(1, X)
#define atomic_fetch_dec_release(X) atomic_fetch_sub_release(1, X)
#define atomic_fetch_and(V, X) __LK_FULL(__lk_ret(X, fetch_and, V, __ATOMIC_RELAXED))
#define atomic_fetch_and_relaxed(V, X) __lk_ret(X, fetch_and, V, __ATOMIC_RELAXED)
#define atomic_fetch_and_acquire(V, X) __lk_ret(X, fetch_and, V, __ATOMIC_ACQUIRE)
#define atomic_fetch_and_release(V, X) __lk_ret(X, fetch_and, V, __ATOMIC_RELEASE)
#define atomic_fetch_or(V, X) __LK_FULL(__lk_ret(X, fetch_or, V, __ATOMIC_RELAXED))
#define atomic_fetch_or_relaxed(V, X) __lk_ret(X, fetch_or, V, __ATOMIC_RELAXED)
#define atomic_fetch_or_acquire(V, X) __lk_ret(X, fetch_or, V, __ATOMIC_ACQUIRE)
#define atomic_fetch_or_release(V, X) __lk_ret(X, fetch_or, V, __ATOMIC_RELEASE)
#define atomic_fetch_xor(V, X) __LK_FULL(__lk_ret(X, fetch_xor, V, __ATOMIC_RELAXED))
#define atomic_fetch_xor_relaxed(V, X) __lk_ret(X, fetch_xor, V, __ATOMIC_RELAXED)
#define atomic_fetch_xor_acquire(V, X) __lk_ret(X, fetch_xor, V, __ATOMIC_ACQUIRE)
#define atomic_fetch_xor_release(V, X) __lk_ret(X, fetch_xor, V, __ATOMIC_RELEASE)
#define atomic_fetch_andnot(V, X) atomic_fetch_and(~(V), X)
#define atomic_fetch_andnot_relaxed(V, X) atomic_fetch_and_relaxed(~(V), X)
#define atomic_fetch_andnot_acquire(V, X) atomic_fetch_and_acquire(~(V), X)
#define atomic_fetch_andnot_release(V, X) atomic_fetch_and_release(~(V), X)
#define atomic_xchg(X, V) xchg(&(X)->counter, V)
#define atomic_xchg_relaxed(X, V) xchg_relaxed(&(X)->counter, V)
#define atomic_xchg_acquire(X, V) xchg_acquire(&(X)->counter, V)
#define atomic_xchg_release(X, V) xchg_release(&(X)->counter, V)
#define atomic_cmpxchg(X, V, W) cmpxchg(&(X)->counter, V, W)
#define atomic_cmpxchg_relaxed(X, V, W) cmpxchg_relaxed(&(X)->counter, V, W)
#define atomic_cmpxchg_acquire(X, V, W) cmpxchg_acquire(&(X)->counter, V, W)
#define atomic_cmpxchg_release(X, V, W) cmpxchg_release(&(X)->counter, V, W)
#define atomic_sub_and_test(V, X) (atomic_sub_return(V, X) == 0)
#define atomic_dec_and_test(X) (atomic_dec_return(X) == 0)
#define atomic_inc_and_test(X) (atomic_inc_return(X) == 0)
#define atomic_add_negative(V, X) (atomic_add_return(V, X) < 0)
#define atomic_add_negative_relaxed(V, X) (atomic_add_return_relaxed(V, X) < 0)
#define atomic_add_negative_acquire(V, X) (atomic_add_return_acquire(V, X) < 0)
#define atomic_add_negative_release(V, X) (atomic_add_return_release(V, X) < 0)

/* Spinlocks; X is a pointer */
typedef pthread_mutex_t spinlock_t;
#define DEFINE_SPINLOCK(x) spinlock_t x = PTHREAD_MUTEX_INITIALIZER
#define spin_lock(X) pthread_mutex_lock(X)
#define spin_unlock(X) pthread_mutex_unlock(X)
#define spin_trylock(X) (pthread_mutex_trylock(X) == 0)

/* RCU: a reader announces itself with a full fence after setting its flag
 * and before clearing it; synchronize_rcu() fences and waits for every
 * reader flag it finds set */
#ifndef LK_MAX_THREADS
# define LK_MAX_THREADS 16
#endif
static int __lk_rcu_nesting[LK_MAX_THREADS]; /* each slot used by one thread */
static int __lk_rcu_reader[LK_MAX_THREADS];

#define rcu_read_lock() do {							\
	if (__lk_rcu_nesting[__LK_TID]++ == 0) {				\
		__atomic_store_n(&__lk_rcu_reader[__LK_TID], 1, __ATOMIC_RELAXED); \
		smp_mb();							\
	}									\
} while (0)
#define rcu_read_unlock() do {							\
	if (--__lk_rcu_nesting[__LK_TID] == 0) {				\
		smp_mb();							\
		__atomic_store_n(&__lk_rcu_reader[__LK_TID], 0, __ATOMIC_RELAXED); \
	}									\
} while (0)
#define synchronize_rcu() do {							\
	smp_mb();								\
	for (int __lk_i = 0; __lk_i < LK_MAX_THREADS; __lk_i++)		\
		if (__lk_i != __LK_TID)						\
			__VERIFIER_await(__atomic_load_n(&__lk_rcu_reader[__lk_i], \
							 __ATOMIC_RELAXED) == 0); \
	smp_mb();								\
} while (0)
#define synchronize_rcu_expedited() synchronize_rcu()

#endif /* LKMM_C11_H */
