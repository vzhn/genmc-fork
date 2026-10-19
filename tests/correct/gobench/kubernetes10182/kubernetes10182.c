#include "../../../lib/go.h"

/*
 * GoBench kernel kubernetes#10182 (blocking, channel and lock), after the
 * kubelet's statusManager. SetPodStatus() takes podStatusesLock for writing
 * and, holding it, sends a sync request on the unbuffered podStatusChannel.
 * The sync loop receives a request and, when the update fails, deletes the
 * cached status, which takes podStatusesLock.
 *
 * With two SetPodStatus() calls, the second can take the lock and block
 * sending while the sync loop, done with the first request, waits for the
 * lock to delete its status.
 *
 * BUG_SYNC_DELETE: the sync loop deletes the status synchronously, as
 * before Kubernetes commit 64ad3e17ad15 ("Fix deadlock", 2015), which
 * deletes it in a new goroutine.
 */

go_rwmutex_t pod_statuses_lock;
go_chan_t *pod_status_channel;

static void delete_pod_status(void)
{
	go_rwmutex_lock(&pod_statuses_lock);
	go_rwmutex_unlock(&pod_statuses_lock);
}

void *delete_pod_status_go(void *arg)
{
	delete_pod_status();
	return NULL;
}

static void sync_batch(void)
{
	go_recv(pod_status_channel);
	/* updating the status failed: drop the cached value */
#ifdef BUG_SYNC_DELETE
	delete_pod_status();
#else
	go(delete_pod_status_go, NULL);
#endif
}

/* G1: s.Start() */
void *start(void *arg)
{
	for (int i = 0; i < 2; i++)
		sync_batch();
	return NULL;
}

/* G2, G3 */
void *set_pod_status(void *arg)
{
	go_rwmutex_lock(&pod_statuses_lock);
	go_send(pod_status_channel, 1);
	go_rwmutex_unlock(&pod_statuses_lock);
	return NULL;
}

int main()
{
	go_init();
	pod_status_channel = go_make_chan(0);
	go(start, NULL);
	go(set_pod_status, NULL);
	go(set_pod_status, NULL);
	return 0;
}
