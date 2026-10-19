#include "../../../lib/go.h"

/*
 * GoBench kernel moby#33781 (blocking, channel and context), after Docker's
 * container health-check monitor. On each probe interval the monitor starts
 * a probe goroutine, which sends its result on an unbuffered results
 * channel, and waits for the result, the probe's timeout or a stop request.
 * Another goroutine sends the stop request.
 *
 * If the stop request arrives while a probe runs, the monitor returns and
 * nobody receives the probe's result: the probe goroutine blocks forever.
 * The model runs one probe, starting at once; after it, the monitor only
 * waits for stop.
 *
 * BUG_UNDRAINED_RESULTS: on stop, the monitor returns without receiving the
 * result, from an unbuffered channel, as before moby commit 67297ba0051d
 * ("Prevent a goroutine leak when healthcheck gets stopped", 2017), which
 * buffers the channel and waits for the probe.
 */

go_chan_t *stop;

void *probe(void *arg)
{
	go_chan_t *results = arg;

	go_send(results, 1);
	go_close(results);
	return NULL;
}

/* G1 */
void *monitor(void *arg)
{
	/* the first probe interval has passed: run a probe */
#ifdef BUG_UNDRAINED_RESULTS
	go_chan_t *results = go_make_chan(0);
#else
	go_chan_t *results = go_make_chan(1);
#endif
	go_ctx_t *ctx = go_ctx_with_timeout(go_ctx_background());
	go(probe, results);

	struct go_case cs[] = { GO_RECV(stop, NULL), GO_RECV(results, NULL),
				GO_RECV(go_ctx_done(ctx), NULL) };
	switch (go_select(cs, 3, false)) {
	case 0:
		go_ctx_cancel(ctx);
#ifndef BUG_UNDRAINED_RESULTS
		go_recv(results);
#endif
		return NULL;
	case 1:
		go_ctx_cancel(ctx);
		break;
	case 2:
		go_ctx_cancel(ctx);
		go_recv(results);
		break;
	}
	go_recv(stop);
	return NULL;
}

/* G2 */
void *stopper(void *arg)
{
	go_send(stop, 1);
	return NULL;
}

int main()
{
	go_init();
	stop = go_make_chan(0);
	go(monitor, NULL);
	go(stopper, NULL);
	return 0;
}
