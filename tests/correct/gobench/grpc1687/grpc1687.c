#include "../../../lib/go.h"

/*
 * GoBench kernel grpc#1687 (non-blocking, misused channel), after grpc-go's
 * serverHandlerTransport. Writes are functions sent on the writes channel
 * to the stream goroutine, unless the transport is closed (closedCh).
 * WriteStatus() sends a last write and closes writes. A handler that writes
 * after its status sends on the closed channel: a panic in Go (an
 * assertion here).
 *
 * BUG_WRITES_CLOSED_TRANSPORT_OPEN: WriteStatus() closes writes without
 * closing the transport, as before grpc-go PR #1687 ("transport: fix race
 * sending RPC status that could lead to a panic", 2017), which closes the
 * transport first, so later writes see closedCh and return.
 */

go_chan_t *closed_ch;
go_chan_t *writes;

static void do_write(void)
{
	struct go_case first[] = { GO_RECV(closed_ch, NULL) };

	if (go_select(first, 1, true) == 0)
		return;
	struct go_case cs[] = { GO_SEND(writes, 1), GO_RECV(closed_ch, NULL) };
	go_select(cs, 2, false);
}

static void write_status(void)
{
	do_write();
#ifndef BUG_WRITES_CLOSED_TRANSPORT_OPEN
	go_close(closed_ch);	/* ht.Close() */
#endif
	go_close(writes);
}

/* the handler, started by HandleStreams() */
void *handle_stream(void *arg)
{
	write_status();
	do_write();		/* st.ht.Write() */
	return NULL;
}

/* ht.runStream() */
static void run_stream(void)
{
	for (;;) {
		bool ok;
		struct go_case cs[] = { GO_RECV2(writes, NULL, &ok), GO_RECV(closed_ch, NULL) };

		if (go_select(cs, 2, false) == 1 || !ok)
			return;
	}
}

int main()
{
	go_init();
	closed_ch = go_make_chan(0);
	writes = go_make_chan(0);
	go(handle_stream, NULL);
	run_stream();
	return 0;
}
