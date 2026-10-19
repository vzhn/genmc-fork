#include "../../../lib/go.h"

/*
 * GoBench kernel cockroach#2448 (blocking, channel), after CockroachDB's
 * early multiraft. The raft state goroutine runs callbacks it receives on
 * callbackChan and, when idle, sends committed events to the store on the
 * unbuffered Events channel. The store's goroutine receives events and runs
 * each one's callback, which sends a function back on callbackChan for the
 * state goroutine to run.
 *
 * With a second event ready, the state goroutine blocks sending it on
 * Events while the store, inside the first event's callback, blocks
 * sending on callbackChan. Both also watch the stopper, but here the test
 * stops only once both callbacks have run.
 *
 * BUG_EVENT_SEND_OUTSIDE_SELECT: the state goroutine sends events with a
 * blocking send from its idle path, as before CockroachDB PR #2448 (2015),
 * which sends pending events from the state loop's main select, next to
 * receiving callbacks.
 */

#define NEVENTS 2

go_chan_t *done;		/* the stopper */
go_chan_t *events;
go_chan_t *callback_chan;
atomic_int processed;

/* the callback's function, run by the state goroutine */
static void callback_fn(void)
{
	atomic_fetch_add(&processed, 1);
}

/* an event's callback, run by the store */
static void callback(void)
{
	struct go_case cs[] = { GO_SEND(callback_chan, 1), GO_RECV(done, NULL) };
	go_select(cs, 2, false);
}

/* G1: s.processRaft() */
void *process_raft(void *arg)
{
	for (;;) {
		struct go_case cs[] = { GO_RECV(events, NULL), GO_RECV(done, NULL) };

		if (go_select(cs, 2, false) == 1)
			return NULL;
		callback();
	}
}

/* G2: st.start() */
void *state_start(void *arg)
{
	int pending = NEVENTS;

	for (;;) {
#ifdef BUG_EVENT_SEND_OUTSIDE_SELECT
		struct go_case cs[] = { GO_RECV(done, NULL), GO_RECV(callback_chan, NULL) };
		int i = go_select(cs, 2, pending > 0);

		if (i == 0)
			return NULL;
		if (i == 1) {
			callback_fn();
			continue;
		}
		/* default: handleWriteResponse() -> sendEvent() */
		struct go_case send[] = { GO_SEND(events, 1), GO_RECV(done, NULL) };
		if (go_select(send, 2, false) == 1)
			return NULL;
		pending--;
#else
		struct go_case cs[] = { GO_RECV(done, NULL), GO_RECV(callback_chan, NULL),
					GO_SEND(pending > 0 ? events : NULL, 1) };
		switch (go_select(cs, 3, false)) {
		case 0:
			return NULL;
		case 1:
			callback_fn();
			break;
		case 2:
			pending--;
			break;
		}
#endif
	}
}

int main()
{
	go_init();
	done = go_make_chan(0);
	events = go_make_chan(0);
	callback_chan = go_make_chan(0);
	go(process_raft, NULL);
	go(state_start, NULL);
	__VERIFIER_await(atomic_load(&processed) == NEVENTS);
	go_close(done);
	return 0;
}
