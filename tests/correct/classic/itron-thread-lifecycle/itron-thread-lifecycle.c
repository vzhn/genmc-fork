#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * Rust std's threads on the ITRON-based kmc-solid targets
 * (library/std/src/sys/itron/thread.rs). Parent and child share a
 * ThreadInner, freed by whichever of them finishes with it last; one word,
 * lifecycle, decides who that is:
 *
 *   INIT --child finishes--> FINISHED --parent joins/detaches--> (parent frees)
 *   INIT --parent detaches--> DETACHED --child finishes--> (child frees)
 *   INIT --parent joins--> JOINING(parent's task) --child finishes-->
 *        JOIN_FINALIZE, child wakes the parent --> (parent frees)
 *
 * FINISHED, DETACHED and JOIN_FINALIZE share one value; every transition is
 * a swap. In JOINING the word holds the joiner's task ID, which the child
 * uses to wake it (wup_tsk). Here a task ID is a pointer to the task's
 * control block, which the parent sets up when it first asks for its own
 * ID; the kernel's sleep and wake-up calls are an await and a store on it.
 *
 * Main spawns two threads, joins the first and detaches the second.
 *
 * BUG_DETACH_ACQUIRE: detaching swapped with acquire and the child's final
 * swap was release only, as before commit 6ad83834515912c5 ("kmc-solid: Fix
 * memory ordering in thread operations", Rust PR #105590, 2022). When the
 * parent detaches first, nothing orders the parent's swap, an access to
 * ThreadInner, before the child frees it. The fix made the swaps acq_rel;
 * the switch undoes the parent's side, which alone brings the bug back.
 */

#define LIFECYCLE_INIT			0UL
#define LIFECYCLE_DONE			(~0UL)	/* FINISHED, DETACHED, JOIN_FINALIZE */

#ifdef BUG_DETACH_ACQUIRE
# define DETACH_ORDER memory_order_acquire
#else
# define DETACH_ORDER memory_order_acq_rel
#endif

struct task {
	int priority;
	atomic_bool wakeup;	/* the kernel's wake-up request count */
};

struct thread_inner {
	int start;		/* the closure, taken by the child */
	atomic_ulong lifecycle;
};

struct thread {
	struct thread_inner *inner;
	pthread_t task;
};

static struct task *main_task;

/* task::current_task_id(): the control block is set up on first use */
static struct task *current_task(void)
{
	if (!main_task) {
		main_task = malloc(sizeof(*main_task));
		main_task->priority = 1;
		atomic_init(&main_task->wakeup, false);
	}
	return main_task;
}

static void wup_tsk(struct task *t)
{
	assert(t->priority == 1);
	atomic_store_explicit(&t->wakeup, true, memory_order_release);
}

static void slp_tsk(struct task *t)
{
	__VERIFIER_await(atomic_load_explicit(&t->wakeup, memory_order_acquire));
	atomic_store_explicit(&t->wakeup, false, memory_order_relaxed);
}

/* the child's entry point */
void *trampoline(void *arg)
{
	struct thread_inner *inner = arg;
	int closure = inner->start;

	assert(closure == 7);

	unsigned long old = atomic_exchange_explicit(&inner->lifecycle, LIFECYCLE_DONE,
						     memory_order_acq_rel);
	if (old == LIFECYCLE_DONE) {
		/* [DETACHED -> EXITED]: the child owns inner now */
		free(inner);
	} else if (old != LIFECYCLE_INIT) {
		/* [JOINING -> JOIN_FINALIZE]: wake the joiner */
		wup_tsk((struct task *) old);
	}
	/* [INIT -> FINISHED]: the parent will free inner */
	return NULL;
}

static struct thread thread_new(void)
{
	struct thread t;

	t.inner = malloc(sizeof(*t.inner));
	t.inner->start = 7;
	atomic_init(&t.inner->lifecycle, LIFECYCLE_INIT);
	pthread_create(&t.task, NULL, trampoline, t.inner);
	return t;
}

static void thread_join(struct thread t)
{
	struct task *me = current_task();
	unsigned long old = atomic_exchange_explicit(&t.inner->lifecycle, (unsigned long) me,
						     memory_order_acq_rel);

	if (old == LIFECYCLE_INIT) {
		/* [INIT -> JOINING]: sleep until the child finalizes the join */
		do {
			slp_tsk(me);
		} while (atomic_load_explicit(&t.inner->lifecycle, memory_order_acquire) !=
			 LIFECYCLE_DONE);
	}
	/* [FINISHED or JOIN_FINALIZE -> JOINED] */
	free(t.inner);
}

static void thread_drop(struct thread t)
{
	unsigned long old = atomic_exchange_explicit(&t.inner->lifecycle, LIFECYCLE_DONE,
						     DETACH_ORDER);

	if (old == LIFECYCLE_DONE)
		free(t.inner);	/* [FINISHED -> JOINED] */
	/* else [INIT -> DETACHED]: the child will free inner */
}

int main()
{
	struct thread joined = thread_new();
	struct thread detached = thread_new();

	thread_join(joined);
	thread_drop(detached);
	return 0;
}
