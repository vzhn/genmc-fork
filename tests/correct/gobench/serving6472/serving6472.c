#include "../../../lib/go.h"

/*
 * GoBench kernel serving#6472 (non-blocking, data race), after Knative
 * Serving's ingress status Prober. IsReady() groups the probe targets into
 * work items sharing an ingressState, queues them for the prober's
 * workers, and then adds the number of items to the state's pendingCount.
 * A worker that has already taken an item decrements pendingCount
 * atomically.
 *
 * BUG_COUNT_AFTER_QUEUEING: pendingCount is updated with a plain add after
 * the items are queued, as before Knative Serving PR #6472 (2019), which
 * sets it before queueing them.
 */

#define NWORKERS 2

struct ingress_state {
	int pending_count;
};

struct work_queue {
	go_mutex_t mu;
	struct ingress_state *items[2];
	int n;
};

struct work_queue queue;
struct ingress_state ingress, initial;
go_waitgroup_t wg;

static void add(struct ingress_state *item)
{
	go_mutex_lock(&queue.mu);
	queue.items[queue.n++] = item;
	go_mutex_unlock(&queue.mu);
}

static struct ingress_state *get(void)
{
	struct ingress_state *item = NULL;

	go_mutex_lock(&queue.mu);
	if (queue.n > 0)
		item = queue.items[--queue.n];
	go_mutex_unlock(&queue.mu);
	return item;
}

/* the prober's workers: processWorkItem() -> updateStates() */
void *worker(void *arg)
{
	struct ingress_state *s = get();

	if (s)
		__atomic_fetch_sub(&s->pending_count, 1, __ATOMIC_SEQ_CST);
	go_wg_done(&wg);
	return NULL;
}

static void is_ready(void)
{
	int nitems = 1;
#ifdef BUG_COUNT_AFTER_QUEUEING
	add(&ingress);
	ingress.pending_count += nitems;
#else
	__atomic_store_n(&ingress.pending_count, nitems, __ATOMIC_SEQ_CST);
	add(&ingress);
#endif
}

int main()
{
	go_init();
	add(&initial);		/* NewProber() queues an item */
	initial.pending_count = 1;
	go_wg_add(&wg, NWORKERS);
	for (int i = 0; i < NWORKERS; i++)
		go(worker, NULL);	/* prober.Start() */
	is_ready();
	go_wg_wait(&wg);
	return 0;
}
