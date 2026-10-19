#include "../../../lib/go.h"

/*
 * GoBench kernel etcd#7902 (blocking, channel and lock), after etcd's
 * election runner (tools/functional-tester/etcd-runner). Round clients run
 * rounds: each validates (records the round's nextc channel) and releases;
 * in release, the leader closes nextc and makes a new one, and every
 * client then waits for the nextc it recorded to close. Validate and
 * release each ran under one mutex shared by all clients.
 *
 * A follower that enters release first holds the mutex while it waits for
 * nextc; the leader needs the mutex to close it.
 *
 * NCLIENTS clients (client 0 is the leader) run NROUNDS rounds. As in the
 * runner, the leader releases a round only once every client has
 * validated it. nextc is atomic in the model, where Go's version shares it
 * unsynchronised.
 *
 * BUG_RELEASE_UNDER_MUTEX: release runs under the shared mutex, as before
 * etcd commit 87d99fe0387e ("etcd-runner: remove mutex on validate() and
 * release() in global.go", 2017).
 */

#ifndef NCLIENTS
# define NCLIENTS 2
#endif
#ifndef NROUNDS
# define NROUNDS 1
#endif

_Atomic(go_chan_t *) nextc;
atomic_int validated;
go_mutex_t mu;
go_waitgroup_t wg;

void *round_client(void *arg)
{
	long id = (long) arg;
	go_chan_t *rc_nextc = NULL;

	for (int progress = 0; progress < NROUNDS; progress++) {
		go_mutex_lock(&mu);
		rc_nextc = atomic_load(&nextc);		/* validate() */
		atomic_fetch_add(&validated, 1);
		go_mutex_unlock(&mu);
#ifdef BUG_RELEASE_UNDER_MUTEX
		go_mutex_lock(&mu);
#endif
		if (id == 0) {				/* release() */
			/* the round's validateWaiters: wait for every client */
			__VERIFIER_await(atomic_load(&validated) == NCLIENTS * (progress + 1));
			go_close(atomic_load(&nextc));
			atomic_store(&nextc, go_make_chan(0));
		}
		go_recv(rc_nextc);
#ifdef BUG_RELEASE_UNDER_MUTEX
		go_mutex_unlock(&mu);
#endif
	}
	go_wg_done(&wg);
	return NULL;
}

int main()
{
	go_init();
	atomic_init(&nextc, go_make_chan(0));
	go_wg_add(&wg, NCLIENTS);
	for (long i = 0; i < NCLIENTS; i++)
		go(round_client, (void *) i);
	go_wg_wait(&wg);
	return 0;
}
