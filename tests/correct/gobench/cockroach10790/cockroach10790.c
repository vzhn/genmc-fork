#include "../../../lib/go.h"

/*
 * GoBench kernel cockroach#10790 (blocking, channel and context), after
 * CockroachDB's Replica.beginCmds(). A command waits, in turn, for each
 * prerequisite command's done channel, unless its context is cancelled;
 * then it hands the waiting to a goroutine that still receives from every
 * prerequisite channel. A helper sends on the prerequisite channels until
 * the context is cancelled; another goroutine cancels it, and main shuts
 * the stopper down. One prerequisite (NCHANS) is enough for the bug.
 *
 * Once the context is cancelled, the helper stops sending, so the waiting
 * goroutine blocks forever.
 *
 * BUG_DRAIN_IGNORES_QUIESCE: the waiting goroutine only receives from the
 * prerequisite channels, as before CockroachDB commit f1a5c19125c6 ("kv:
 * fix goroutine leak on shutdown", 2016), which also selects on the
 * stopper's quiesce channel.
 */

#ifndef NCHANS
# define NCHANS 1
#endif

struct stopper {
	go_chan_t *quiescer;
	go_mutex_t mu;
	bool quiescing;
};

struct stopper stopper;
go_chan_t *chans[NCHANS];
go_ctx_t *ctx;

static void quiesce(void)
{
	go_mutex_lock(&stopper.mu);
	if (!stopper.quiescing) {
		stopper.quiescing = true;
		go_close(stopper.quiescer);
	}
	go_mutex_unlock(&stopper.mu);
}

void *wait_prerequisites(void *arg)
{
	for (int i = 0; i < NCHANS; i++) {
#ifdef BUG_DRAIN_IGNORES_QUIESCE
		go_recv(chans[i]);
#else
		struct go_case cs[] = { GO_RECV(chans[i], NULL), GO_RECV(stopper.quiescer, NULL) };
		if (go_select(cs, 2, false) == 1)
			return NULL;
#endif
	}
	return NULL;
}

/* G1: r.beginCmds(ctx) */
void *begin_cmds(void *arg)
{
	go_chan_t *ctx_done = go_ctx_done(ctx);

	for (int i = 0; i < NCHANS; i++) {
		struct go_case cs[] = { GO_RECV(chans[i], NULL), GO_RECV(ctx_done, NULL) };
		if (go_select(cs, 2, false) == 1)
			go(wait_prerequisites, NULL);
	}
	return NULL;
}

/* the helper: r.sendChans(ctx) */
void *send_chans(void *arg)
{
	for (int i = 0; i < NCHANS; i++) {
		struct go_case cs[] = { GO_SEND(chans[i], 1), GO_RECV(go_ctx_done(ctx), NULL) };
		if (go_select(cs, 2, false) == 1)
			return NULL;
	}
	return NULL;
}

/* G2 */
void *cancel(void *arg)
{
	go_ctx_cancel(ctx);
	return NULL;
}

int main()
{
	go_init();
	stopper.quiescer = go_make_chan(0);
	for (int i = 0; i < NCHANS; i++)
		chans[i] = go_make_chan(0);
	ctx = go_ctx_with_cancel(go_ctx_background());
	go(send_chans, NULL);
	go(begin_cmds, NULL);
	go(cancel, NULL);
	quiesce();		/* r.stopper.Stop() */
	return 0;
}
