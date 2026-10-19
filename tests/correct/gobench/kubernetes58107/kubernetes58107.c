#include "../../../lib/go.h"

/*
 * GoBench kernel kubernetes#58107 (blocking, RWR deadlock), after the
 * Kubernetes resource quota controller. Two workers each pull items from
 * their own work queue (queue and missingUsageQueue) and process them;
 * a third goroutine periodically takes the controller's workerLock for
 * writing to resync.
 *
 * A work queue's Get() waits on a sync.Cond until an item arrives or the
 * queue is shut down. Here the first queue stays empty and the second gets
 * one item; once that item is processed, main shuts both queues down.
 *
 * Go's RWMutex makes new readers wait behind a waiting writer. If worker 1
 * waits in Get() holding the read lock, the resync's Lock() waits for it,
 * worker 2's RLock() waits behind the resync, and the item on queue 2 is
 * never processed.
 *
 * BUG_RLOCK_AROUND_GET: the workers hold workerLock for reading while they
 * wait in Get(), as before Kubernetes commit 010a127314a9 (2018), which
 * takes the read lock only after Get() returns.
 */

struct queue {
	go_mutex_t mu;
	go_cond_t cond;
	int items;
	bool shutting_down;
};

go_rwmutex_t worker_lock;
struct queue queue, missing_usage_queue;
atomic_int processed;

/* returns false once the queue is shut down */
static bool get(struct queue *q)
{
	go_mutex_lock(&q->mu);
	while (q->items == 0 && !q->shutting_down)
		go_cond_wait(&q->cond, &q->mu);
	bool got = q->items > 0;
	if (got)
		q->items--;
	go_mutex_unlock(&q->mu);
	return got;
}

static void add(struct queue *q)
{
	go_mutex_lock(&q->mu);
	q->items++;
	go_cond_signal(&q->cond);
	go_mutex_unlock(&q->mu);
}

static void shut_down(struct queue *q)
{
	go_mutex_lock(&q->mu);
	q->shutting_down = true;
	go_cond_broadcast(&q->cond);
	go_mutex_unlock(&q->mu);
}

/* rq.worker(queue)(): one pass of workFunc() per item */
void *worker(void *arg)
{
	struct queue *q = arg;

	for (;;) {
#ifdef BUG_RLOCK_AROUND_GET
		go_rwmutex_rlock(&worker_lock);
		bool got = get(q);
		if (got)
			atomic_fetch_add(&processed, 1);	/* syncResourceQuota */
		go_rwmutex_runlock(&worker_lock);
#else
		bool got = get(q);
		if (got) {
			go_rwmutex_rlock(&worker_lock);
			atomic_fetch_add(&processed, 1);
			go_rwmutex_runlock(&worker_lock);
		}
#endif
		if (!got)
			return NULL;
	}
}

/* rq.Sync(): one resync */
void *sync_quotas(void *arg)
{
	go_rwmutex_lock(&worker_lock);
	go_rwmutex_unlock(&worker_lock);
	return NULL;
}

int main()
{
	go_init();
	go(worker, &queue);
	go(worker, &missing_usage_queue);
	go(sync_quotas, NULL);

	add(&missing_usage_queue);
	__VERIFIER_await(atomic_load(&processed) == 1);
	shut_down(&queue);
	shut_down(&missing_usage_queue);
	return 0;
}
