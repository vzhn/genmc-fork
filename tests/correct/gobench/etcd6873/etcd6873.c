#include "../../../lib/go.h"

/*
 * GoBench kernel etcd#6873 (blocking, channel and lock), after etcd's
 * grpcproxy.watchBroadcasts. A goroutine coalesces the broadcasts sent on
 * updatec (buffer of one), taking wbs.mu for each, and closes donec when
 * updatec is closed. update() sends on updatec unless it is full; stop()
 * takes wbs.mu, closes updatec and waits for donec.
 *
 * If the coalescing goroutine is about to take wbs.mu for an update when
 * stop() takes it, stop() waits for donec while holding the lock the
 * goroutine needs.
 *
 * BUG_STOP_WAITS_UNDER_LOCK: stop() waits for donec with wbs.mu held, as
 * before etcd commit 7618fdd1d642 ("grpcproxy: fix deadlock on watch
 * broadcasts stop", 2016), which unlocks first.
 */

go_mutex_t mu;
go_chan_t *updatec;
go_chan_t *donec;

static void coalesce(long wb)
{
	go_mutex_lock(&mu);
	go_mutex_unlock(&mu);
}

/* G2: for wb := range wbs.updatec { wbs.coalesce(wb) }; close(donec) */
void *coalesce_loop(void *arg)
{
	for (;;) {
		bool ok;
		long wb = go_recv2(updatec, &ok);

		if (!ok)
			break;
		coalesce(wb);
	}
	go_close(donec);
	return NULL;
}

static void update(long wb)
{
	struct go_case cs[] = { GO_SEND(updatec, wb) };

	go_select(cs, 1, true);
}

/* G3 */
void *stop(void *arg)
{
	go_mutex_lock(&mu);
	go_close(updatec);
#ifdef BUG_STOP_WAITS_UNDER_LOCK
	go_recv(donec);
	go_mutex_unlock(&mu);
#else
	go_mutex_unlock(&mu);
	go_recv(donec);
#endif
	return NULL;
}

int main()
{
	go_init();
	updatec = go_make_chan(1);
	donec = go_make_chan(0);
	go(coalesce_loop, NULL);
	update(1);
	go(stop, NULL);
	return 0;
}
