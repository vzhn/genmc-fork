#include "../../../lib/go.h"

/*
 * GoBench kernel kubernetes#70277 (blocking, channel), after
 * k8s.io/apimachinery's wait.WaitFor(). WaitFor() starts a poller with a
 * done channel; the poller ticks on a channel until done closes or its
 * timeout fires, and WaitFor() calls the condition after each tick. The
 * test passes the caller's stop channel as done, captures it, and after
 * WaitFor() returns waits for done to close; the caller closes its stop
 * channel only on return (defer).
 *
 * WaitFor() returns as soon as the condition holds, without stopping the
 * poller, and the test then waits for a channel nobody will close.
 *
 * BUG_POLLER_USES_CALLER_DONE: WaitFor() hands the caller's done channel to
 * the poller, as before Kubernetes PR #70277 (2018), which gives the
 * poller its own stop channel and closes it when WaitFor() returns.
 */

go_chan_t *ticker;

struct poller_arg {
	go_chan_t *ch;
	go_chan_t *done;
};

struct poller_arg pa;

void *tick(void *arg)
{
	go_send(ticker, 1);	/* one tick */
	return NULL;
}

void *poller(void *arg)
{
	go_chan_t *after = go_time_after();

	for (;;) {
		struct go_case cs[] = { GO_RECV(ticker, NULL), GO_RECV(after, NULL),
					GO_RECV(pa.done, NULL) };
		int i = go_select(cs, 3, false);

		if (i == 0) {
			struct go_case send[] = { GO_SEND(pa.ch, 1) };
			go_select(send, 1, true);
		} else {
			break;
		}
	}
	go_close(pa.ch);
	return NULL;
}

/* the WaitFunc: waitFunc(done), capturing done */
static go_chan_t *done_ch;

static go_chan_t *wait_func(go_chan_t *done)
{
	done_ch = done;
	pa.ch = go_make_chan(0);
	pa.done = done;
	go(poller, NULL);
	return pa.ch;
}

static void wait_for(go_chan_t *done)
{
#ifdef BUG_POLLER_USES_CALLER_DONE
	go_chan_t *c = wait_func(done);
#else
	go_chan_t *stop_ch = go_make_chan(0);
	go_chan_t *c = wait_func(stop_ch);
#endif
	go_recv(c);		/* the first tick; the condition holds at once */
#ifndef BUG_POLLER_USES_CALLER_DONE
	go_close(stop_ch);		/* defer closeCh() */
#endif
}

int main()
{
	go_init();
	ticker = go_make_chan(1);
	go(tick, NULL);

	go_chan_t *stop_ch = go_make_chan(0);
	wait_for(stop_ch);
	go_recv(done_ch);
	go_close(stop_ch);		/* defer close(stopCh) */
	return 0;
}
