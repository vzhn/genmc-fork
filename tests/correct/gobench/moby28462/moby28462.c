#include "../../../lib/go.h"

/*
 * GoBench kernel moby#28462 (blocking, channel and lock), after Docker's
 * container health checks. The health monitor loops: it stops if a stop
 * request is pending and otherwise handles a probe result, which takes the
 * container lock. When the container's state changes, the daemon, holding
 * the container lock, stops the monitor (CloseMonitorChannel()).
 *
 * If the monitor is waiting for the container lock when the daemon, holding
 * it, sends the stop request on the unbuffered channel, neither proceeds.
 * The model's monitor handles NPROBES results and then only waits for stop.
 *
 * BUG_SEND_STOP: CloseMonitorChannel() sends on the stop channel, as before
 * moby commit 89b123473774 ("Fix deadlock on cancelling healthcheck",
 * 2016), which closes it instead.
 */

#ifndef NPROBES
# define NPROBES 2
#endif

go_mutex_t container_mu;
go_chan_t *stop;

static void handle_probe_result(void)
{
	go_mutex_lock(&container_mu);
	go_mutex_unlock(&container_mu);
}

/* G1 */
void *monitor(void *arg)
{
	for (int i = 0; i < NPROBES; i++) {
		struct go_case cs[] = { GO_RECV(stop, NULL) };

		if (go_select(cs, 1, true) == 0)
			return NULL;
		handle_probe_result();
	}
	go_recv(stop);
	return NULL;
}

/* G2: d.StateChanged() */
void *state_changed(void *arg)
{
	go_mutex_lock(&container_mu);
#ifdef BUG_SEND_STOP
	go_send(stop, 1);
#else
	go_close(stop);
#endif
	go_mutex_unlock(&container_mu);
	return NULL;
}

int main()
{
	go_init();
	stop = go_make_chan(0);
	go(monitor, NULL);
	go(state_changed, NULL);
	return 0;
}
