#include "../../../lib/go.h"

/*
 * GoBench kernel kubernetes#30872 (blocking, AB-BA deadlock), after the
 * federation namespace controller and its federated informer. Two locks:
 * the federated informer's mutex (A) and the cluster informer's DeltaFIFO
 * lock (B).
 *
 * - ClustersSynced() takes A and asks the cluster informer whether it has
 *   synced, which takes B.
 * - The cluster informer's controller loop pops from the FIFO under B and
 *   processes the item with the add handler, addCluster(), which takes A.
 *
 * The namespace controller starts the federated informer, a goroutine that
 * stops it when the controller's stop channel closes, and a reconcile that
 * checks ClustersSynced(); the test then closes the stop channel. The
 * model's controller loop runs NPOPS times, then waits for stop.
 *
 * BUG_SYNCED_UNDER_INFORMER_LOCK: ClustersSynced() calls HasSynced()
 * holding A, as before Kubernetes PR #30872 (2016), which collects the
 * informers under A and checks them after releasing it.
 */

#ifndef NPOPS
# define NPOPS 1
#endif

go_mutex_t federated_mu;	/* A */
go_rwmutex_t fifo_lock;		/* B */
go_chan_t *informer_stop;	/* f.clusterInformer.stopChan */
go_chan_t *stop;

static void has_synced(void)
{
	go_rwmutex_lock(&fifo_lock);
	go_rwmutex_unlock(&fifo_lock);
}

static void add_cluster(void)
{
	go_mutex_lock(&federated_mu);
	go_mutex_unlock(&federated_mu);
}

/* controller.Run(): JitterUntil(processLoop, stopCh) */
void *controller_run(void *arg)
{
	for (int i = 0; i < NPOPS; i++) {
		struct go_case cs[] = { GO_RECV(informer_stop, NULL) };
		if (go_select(cs, 1, true) == 0)
			return NULL;
		go_rwmutex_lock(&fifo_lock);	/* DeltaFIFO.Pop() */
		add_cluster();			/* h.OnAdd() */
		go_rwmutex_unlock(&fifo_lock);
	}
	go_recv(informer_stop);
	return NULL;
}

static void clusters_synced(void)
{
	go_mutex_lock(&federated_mu);
#ifdef BUG_SYNCED_UNDER_INFORMER_LOCK
	has_synced();
	go_mutex_unlock(&federated_mu);
#else
	go_mutex_unlock(&federated_mu);
	has_synced();
#endif
}

void *stopper(void *arg)
{
	go_recv(stop);
	go_mutex_lock(&federated_mu);	/* Stop() */
	go_close(informer_stop);
	go_mutex_unlock(&federated_mu);
	return NULL;
}

void *reconcile(void *arg)
{
	clusters_synced();
	return NULL;
}

int main()
{
	go_init();
	stop = go_make_chan(0);

	/* namespaceController.Run(stop) */
	go_mutex_lock(&federated_mu);	/* Start() */
	informer_stop = go_make_chan(0);
	go(controller_run, NULL);
	go_mutex_unlock(&federated_mu);
	go(stopper, NULL);
	go(reconcile, NULL);		/* StartWithHandler() */

	go_close(stop);
	return 0;
}
