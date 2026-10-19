#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * Rust's std::thread::scope (library/std/src/thread/scoped.rs). The scope's
 * ScopeData counts the scoped threads still running and holds a handle to
 * the main thread. Each scoped thread, when it finishes, decrements the
 * count with a release; the thread that brings it to zero unparks the main
 * thread. scope() waits with an acquire load until the count is zero, then
 * returns, and the ScopeData goes away.
 *
 * park() may return spuriously, so the wait is a loop on the count; here it
 * is an await. The ScopeData is heap-allocated so that its end is a free.
 *
 * Main opens a scope, spawns NTHREADS scoped threads that each read a
 * borrowed value, and leaves the scope.
 *
 * BUG_SCOPE_DATA_ON_STACK: the ScopeData lived in scope()'s stack frame, as
 * before commit ebecc1310600 ("fix data race in thread::scope", Rust PR
 * #98503, 2022; found by Miri, issue #98498). The last thread decrements the
 * count and then reads ScopeData::main_thread to unpark the main thread;
 * main can see the zero count in between, return and drop the ScopeData,
 * so that read is a use after free. The fix puts the ScopeData in an Arc
 * that every scoped thread keeps until it has finished with it. The bug
 * is in the interleaving, not in the orderings: it fails under SC too.
 */

#ifndef NTHREADS
# define NTHREADS 2
#endif

struct thread {
	atomic_bool unparked;
};

struct scope_data {
	atomic_ulong num_running_threads;
	atomic_bool a_thread_panicked;
	struct thread *main_thread;
	atomic_ulong arc_strong;		/* the Arc of the fix */
};

struct thread main_thread_handle;
int borrowed[NTHREADS];

static void unpark(struct thread *t)
{
	atomic_store_explicit(&t->unparked, true, memory_order_release);
}

static void scope_data_drop_ref(struct scope_data *sd)
{
#ifdef BUG_SCOPE_DATA_ON_STACK
	(void) sd;
#else
	/* Arc::drop() */
	if (atomic_fetch_sub_explicit(&sd->arc_strong, 1, memory_order_release) == 1) {
		atomic_thread_fence(memory_order_acquire);
		free(sd);
	}
#endif
}

static void decrement_num_running_threads(struct scope_data *sd, bool panic)
{
	if (panic)
		atomic_store_explicit(&sd->a_thread_panicked, true, memory_order_relaxed);
	if (atomic_fetch_sub_explicit(&sd->num_running_threads, 1, memory_order_release) == 1)
		unpark(sd->main_thread);
}

void *scoped_thread(void *arg)
{
	struct scope_data *sd = arg;
	static atomic_int next_id;
	int id = atomic_fetch_add_explicit(&next_id, 1, memory_order_relaxed);

	/* the closure */
	assert(borrowed[id] == id + 1);

	/* drop(Packet): the thread is done with 'scope */
	decrement_num_running_threads(sd, false);
	scope_data_drop_ref(sd);
	return NULL;
}

int main()
{
	pthread_t t[NTHREADS];
	struct scope_data *sd = malloc(sizeof(*sd));

	for (int i = 0; i < NTHREADS; i++)
		borrowed[i] = i + 1;

	/* scope() */
	atomic_init(&sd->num_running_threads, 0);
	atomic_init(&sd->a_thread_panicked, false);
	sd->main_thread = &main_thread_handle;
	atomic_init(&sd->arc_strong, 1);

	for (int i = 0; i < NTHREADS; i++) {
		/* Scope::spawn() */
		atomic_fetch_add_explicit(&sd->num_running_threads, 1, memory_order_relaxed);
#ifndef BUG_SCOPE_DATA_ON_STACK
		atomic_fetch_add_explicit(&sd->arc_strong, 1, memory_order_relaxed);
#endif
		pthread_create(&t[i], NULL, scoped_thread, sd);
	}

	/* while num_running_threads != 0 { park() } */
	__VERIFIER_await(atomic_load_explicit(&sd->num_running_threads,
					      memory_order_acquire) == 0);
	assert(!atomic_load_explicit(&sd->a_thread_panicked, memory_order_relaxed));

	/* leave scope(): the ScopeData goes away */
#ifdef BUG_SCOPE_DATA_ON_STACK
	free(sd);
#else
	scope_data_drop_ref(sd);
#endif
	/* the borrow ends: main may reuse the borrowed data */
	for (int i = 0; i < NTHREADS; i++)
		borrowed[i] = 0;
	return 0;
}
