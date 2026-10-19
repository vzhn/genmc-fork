#include "../../../lib/go.h"

/*
 * GoBench kernel cockroach#24808 (blocking, channel), after CockroachDB's
 * storage compactor. Suggested compactions signal the compactor on a
 * channel with a buffer of one. Start() signals it once more before
 * starting the worker that drains the channel; if a suggestion has already
 * filled the buffer, Start() blocks forever, holding up the node's boot.
 *
 * BUG_BLOCKING_START_SIGNAL: Start() sends unconditionally, as before
 * CockroachDB PR #24808 (2018), which skips the send when the buffer is
 * full.
 */

go_chan_t *stopper;
go_chan_t *ch;
go_waitgroup_t stop_wg;

void *worker(void *arg)
{
	for (;;) {
		struct go_case cs[] = { GO_RECV(stopper, NULL), GO_RECV(ch, NULL) };

		if (go_select(cs, 2, false) == 0)
			break;
	}
	go_wg_done(&stop_wg);
	return NULL;
}

static void start(void)
{
#ifdef BUG_BLOCKING_START_SIGNAL
	go_send(ch, 1);
#else
	struct go_case cs[] = { GO_SEND(ch, 1) };
	go_select(cs, 1, true);
#endif
	go_wg_add(&stop_wg, 1);		/* stopper.RunWorker() */
	go(worker, NULL);
}

int main()
{
	go_init();
	stopper = go_make_chan(0);
	ch = go_make_chan(1);
	go_send(ch, 1);			/* a suggestion before Start() */
	start();
	go_close(stopper);		/* defer stopper.Stop() */
	return 0;
}
