#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * glibc's POSIX semaphore (nptl/sem_post.c, sem_wait.c, sem_waitcommon.c)
 * and its destruction. POSIX lets a thread destroy a semaphore, and free
 * its memory, as soon as no thread is blocked on it: in particular right
 * after its own sem_wait() has taken the token that another thread's
 * sem_post() added, even if that sem_post() has not returned yet. So
 * sem_post() must not touch the semaphore after the step that makes the
 * token available.
 *
 * Since glibc 2.21 the token count and the number of waiters share one
 * 64-bit word: sem_post() adds a token with one fetch_add and learns from
 * its result whether to call futex_wake(), which only passes the address
 * to the kernel. sem_wait() takes a token with a CAS, or registers as a
 * waiter and waits on the futex. A futex wait is an await on the token
 * count; futex_wake() does not access memory.
 *
 * The semaphore is on the heap with no tokens. One thread posts; the
 * other waits for the token, destroys the semaphore and frees it.
 *
 * BUG_SEPARATE_NWAITERS: the layout before glibc 2.21, a token count and a
 * separate waiter count. sem_post() adds the token and only then reads the
 * waiter count to decide whether to wake. In between, the waiter can take
 * the token and free the semaphore, and sem_post() reads freed memory
 * (glibc bug 12674, fixed by commit 042e1521c794, "Fix semaphore
 * destruction", 2015). Fails under SC too.
 */

#define SEM_VALUE_MASK		0xffffffffull
#define SEM_NWAITERS_SHIFT	32
#define ONE_WAITER		(1ull << SEM_NWAITERS_SHIFT)

struct new_sem {
#ifdef BUG_SEPARATE_NWAITERS
	atomic_uint value;
	atomic_uint nwaiters;
#else
	atomic_ullong data;	/* tokens in the low half, waiters in the high */
#endif
};

struct new_sem *sem;

static void futex_wake(void *addr) {}

#ifdef BUG_SEPARATE_NWAITERS

static void sem_post(struct new_sem *s)
{
	atomic_fetch_add_explicit(&s->value, 1, memory_order_release);
	if (atomic_load_explicit(&s->nwaiters, memory_order_relaxed) > 0)
		futex_wake(&s->value);
}

static bool try_take(struct new_sem *s)
{
	unsigned v = atomic_load_explicit(&s->value, memory_order_relaxed);

	while (v > 0)
		if (atomic_compare_exchange_strong_explicit(&s->value, &v, v - 1,
							    memory_order_acquire,
							    memory_order_relaxed))
			return true;
	return false;
}

static void sem_wait(struct new_sem *s)
{
	if (try_take(s))
		return;
	atomic_fetch_add_explicit(&s->nwaiters, 1, memory_order_relaxed);
	for (;;) {
		/* futex_wait() until a token shows up */
		__VERIFIER_await(atomic_load_explicit(&s->value, memory_order_relaxed) > 0);
		if (try_take(s))
			break;
	}
	atomic_fetch_sub_explicit(&s->nwaiters, 1, memory_order_relaxed);
}

#else

static void sem_post(struct new_sem *s)
{
	unsigned long long d = atomic_fetch_add_explicit(&s->data, 1, memory_order_release);

	if (d >> SEM_NWAITERS_SHIFT)
		futex_wake(&s->data);
}

static void sem_wait(struct new_sem *s)
{
	unsigned long long d = atomic_load_explicit(&s->data, memory_order_relaxed);

	/* fast path: take a token */
	while (d & SEM_VALUE_MASK)
		if (atomic_compare_exchange_strong_explicit(&s->data, &d, d - 1,
							    memory_order_acquire,
							    memory_order_relaxed))
			return;
	/* slow path: register as a waiter, then take a token and unregister */
	d = atomic_fetch_add_explicit(&s->data, ONE_WAITER, memory_order_relaxed) + ONE_WAITER;
	for (;;) {
		while (d & SEM_VALUE_MASK)
			if (atomic_compare_exchange_strong_explicit(&s->data, &d,
								    d - 1 - ONE_WAITER,
								    memory_order_acquire,
								    memory_order_relaxed))
				return;
		/* futex_wait() until a token shows up */
		__VERIFIER_await(atomic_load_explicit(&s->data, memory_order_relaxed) & SEM_VALUE_MASK);
		d = atomic_load_explicit(&s->data, memory_order_relaxed);
	}
}

#endif

static void *poster(void *arg)
{
	sem_post(sem);
	return NULL;
}

static void *waiter(void *arg)
{
	sem_wait(sem);
	/* sem_destroy() and free: nobody is blocked on the semaphore */
	free(sem);
	return NULL;
}

int main()
{
	pthread_t p, w;

	sem = calloc(1, sizeof(*sem));
	if (!sem)
		abort();
	if (pthread_create(&p, NULL, poster, NULL) ||
	    pthread_create(&w, NULL, waiter, NULL))
		abort();
	if (pthread_join(p, NULL) || pthread_join(w, NULL))
		abort();
	return 0;
}
