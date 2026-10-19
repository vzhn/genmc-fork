#include "../../../lib/go.h"

/*
 * GoBench kernel cockroach#3710 (blocking, recursive read lock), after
 * CockroachDB's storage.Store. ForceRaftLogScanAndProcess() holds the
 * store's RWMutex for reading while it adds each replica to the raft log
 * queue; MaybeAdd() takes the queue's mutex and asks the replica's raft
 * status, which takes the store's read lock again. processRaft() takes the
 * store's lock for writing to copy the replicas.
 *
 * If processRaft()'s Lock() comes between the two RLock()s, the scan waits
 * for the writer and the writer waits for the scan.
 *
 * BUG_MAYBE_ADD_UNDER_RLOCK: the scan calls MaybeAdd() under the read lock,
 * as before CockroachDB commit cb65190f9caa ("Release RLock before calling
 * MaybeAdd to avoid deadlock", 2016), which copies the replicas under the
 * lock and adds them after releasing it.
 */

#define NREPLICAS 2

go_rwmutex_t store_mu;
go_mutex_t base_queue_mu;
int replicas[NREPLICAS] = { 1, 2 };

static void raft_status(void)
{
	go_rwmutex_rlock(&store_mu);
	go_rwmutex_runlock(&store_mu);
}

/* baseQueue.MaybeAdd() -> shouldQueue() -> getTruncatableIndexes() */
static void maybe_add(int r)
{
	go_mutex_lock(&base_queue_mu);
	raft_status();
	go_mutex_unlock(&base_queue_mu);
}

/* G1 */
void *force_raft_log_scan_and_process(void *arg)
{
#ifdef BUG_MAYBE_ADD_UNDER_RLOCK
	go_rwmutex_rlock(&store_mu);
	for (int i = 0; i < NREPLICAS; i++)
		maybe_add(replicas[i]);
	go_rwmutex_runlock(&store_mu);
#else
	int copy[NREPLICAS];

	go_rwmutex_rlock(&store_mu);
	for (int i = 0; i < NREPLICAS; i++)
		copy[i] = replicas[i];
	go_rwmutex_runlock(&store_mu);
	for (int i = 0; i < NREPLICAS; i++)
		maybe_add(copy[i]);
#endif
	return NULL;
}

/* G2 */
void *process_raft(void *arg)
{
	int copy[NREPLICAS];

	go_rwmutex_lock(&store_mu);
	for (int i = 0; i < NREPLICAS; i++)
		copy[i] = replicas[i];
	go_rwmutex_unlock(&store_mu);
	(void) copy;
	return NULL;
}

int main()
{
	go_init();
	go(force_raft_log_scan_and_process, NULL);
	go(process_raft, NULL);
	return 0;
}
