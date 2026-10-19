#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <assert.h>
#include <genmc.h>

/*
 * The task state machine of tokio 0.2.x (tokio/src/task/state.rs and
 * harness.rs): one word holds the task's lifecycle bits and its reference
 * count, and whoever drops the last reference frees the task. A spawned
 * task starts as NOTIFIED | JOIN_INTEREST: it is scheduled, and its
 * JoinHandle may still read the output.
 *
 * - The worker runs the task: transition_to_running() flips RUNNING and
 *   NOTIFIED and reads JOIN_INTEREST. The task stores its output if the
 *   handle still wants it, then completes and releases the task
 *   (transition_to_released()); if that dropped the last reference, the
 *   worker frees the task.
 * - Dropping the JoinHandle first tries drop_join_handle_fast(): a CAS from
 *   NOTIFIED | JOIN_INTEREST to NOTIFIED, which succeeds only while the task
 *   has not started. Otherwise drop_join_handle_slow() clears JOIN_INTEREST,
 *   or, if the task has completed, takes the output and then clears it. The
 *   handle frees the task if that dropped the last reference.
 *
 * Main spawns the task on a worker thread and drops the handle. The task is
 * freed exactly once, after every access to it.
 *
 * BUG_RELAXED_FAST_DROP: drop_join_handle_fast() used a relaxed CAS, as
 * before commit a7a79f28a8fe ("use release ordering in
 * drop_join_handle_fast", 2020, found by loom). If the worker's
 * transition_to_running() reads the CAS's value, it sees no join interest,
 * so the worker drops the last reference and frees the task; but nothing
 * orders the CAS, an access to the task, before that free.
 */

#define RUNNING		0x01UL
#define NOTIFIED	0x02UL
#define COMPLETE	0x04UL
#define RELEASED	0x08UL
#define JOIN_INTEREST	0x10UL
#define JOIN_WAKER	0x20UL
#define CANCELLED	0x40UL
#define LIFECYCLE_MASK	0x7fUL
#define WAKER_ONE	0x80UL

#define INITIAL_STATE	NOTIFIED

#ifdef BUG_RELAXED_FAST_DROP
# define FAST_DROP_ORDER memory_order_relaxed
#else
# define FAST_DROP_ORDER memory_order_release
#endif

struct task {
	atomic_ulong state;
	int output;
};

static bool is_final_ref(unsigned long s)
{
	return (s & (~LIFECYCLE_MASK | RELEASED | JOIN_INTEREST)) == RELEASED;
}

static void dealloc(struct task *t)
{
	free(t);
}

/* ---- the worker ---- */

void *worker(void *arg)
{
	struct task *t = arg;

	/* transition_to_running() */
	unsigned long prev = atomic_fetch_xor_explicit(&t->state, RUNNING | NOTIFIED,
						       memory_order_acquire);
	assert(prev & NOTIFIED);
	bool join_interest = prev & JOIN_INTEREST;

	/* poll the future; complete() stores the output if it is wanted */
	if (join_interest)
		t->output = 42;

	unsigned long res;
	if (join_interest) {
		/* transition_to_complete() */
		res = atomic_fetch_xor_explicit(&t->state, RUNNING | COMPLETE,
						memory_order_acq_rel) ^ (RUNNING | COMPLETE);
		/* notify_join_handle(): the handle lost interest, drop the output */
		if (!(res & JOIN_INTEREST))
			t->output = 0;
		/* release_task() */
		res = atomic_fetch_or_explicit(&t->state, RELEASED, memory_order_release) | RELEASED;
		if (is_final_ref(res))
			atomic_thread_fence(memory_order_acquire);
	} else {
		/* State::transition_to_released() */
		res = atomic_fetch_xor_explicit(&t->state, RUNNING | COMPLETE | RELEASED,
						memory_order_acq_rel) ^
		      (RUNNING | COMPLETE | RELEASED);
	}
	if (is_final_ref(res))
		dealloc(t);
	return NULL;
}

/* ---- the JoinHandle ---- */

static bool drop_join_handle_fast(struct task *t)
{
	unsigned long expected = INITIAL_STATE | JOIN_INTEREST;

	return atomic_compare_exchange_strong_explicit(&t->state, &expected, INITIAL_STATE,
						       FAST_DROP_ORDER, memory_order_relaxed);
}

static void drop_join_handle_slow(struct task *t)
{
	unsigned long prev = atomic_load_explicit(&t->state, memory_order_acquire);
	unsigned long res;

	for (;;) {
		if (prev & (COMPLETE | CANCELLED)) {
			/* the output must be taken first */
			int out = t->output;
			assert(out == 42);
			/* complete_join_handle() */
			res = atomic_fetch_sub_explicit(&t->state, JOIN_INTEREST,
							memory_order_release) - JOIN_INTEREST;
			if (is_final_ref(res))
				atomic_thread_fence(memory_order_acquire);
			break;
		}
		unsigned long next = (prev - JOIN_INTEREST) & ~JOIN_WAKER;
		if (atomic_compare_exchange_strong_explicit(&t->state, &prev, next,
							    memory_order_acq_rel,
							    memory_order_acquire)) {
			res = next;
			break;
		}
	}
	if (is_final_ref(res))
		dealloc(t);
}

int main()
{
	pthread_t w;
	struct task *t = malloc(sizeof(*t));

	atomic_init(&t->state, INITIAL_STATE | JOIN_INTEREST);
	t->output = 0;

	/* tokio::spawn() */
	pthread_create(&w, NULL, worker, t);

	/* drop(JoinHandle) */
	if (!drop_join_handle_fast(t))
		drop_join_handle_slow(t);

	return 0;
}
