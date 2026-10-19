#include "../../../lib/go.h"

/*
 * GoBench kernel syncthing#5795 (blocking, channel), after Syncthing's
 * protocol rawConnection. The dispatcher loop hands received messages to
 * the model; Close() closes the connection once (sync.Once): it closes the
 * closed channel and waits for the dispatcher loop to stop. The test's
 * model closes the connection from its ClusterConfig callback, that is,
 * from inside the dispatcher loop, which then waits for itself.
 *
 * The reader loop polls closed in Go; here it blocks on it.
 *
 * BUG_CLOSE_WAITS_IN_DISPATCHER: Close() runs internalClose() synchronously,
 * as before Syncthing PR #5795 (2019), which runs it in a new goroutine.
 */

go_chan_t *inbox;
go_chan_t *dispatcher_loop_stopped;
go_chan_t *closed;
go_once_t close_once;

static void internal_close_once(void *arg)
{
	go_close(closed);
	go_recv(dispatcher_loop_stopped);
}

static void internal_close(void)
{
	go_once_do(&close_once, internal_close_once, NULL);
}

void *internal_close_go(void *arg)
{
	internal_close();
	return NULL;
}

static void close_connection(void)
{
#ifdef BUG_CLOSE_WAITS_IN_DISPATCHER
	internal_close();
#else
	go(internal_close_go, NULL);
#endif
}

void *reader_loop(void *arg)
{
	go_recv(closed);
	return NULL;
}

void *dispatcher_loop(void *arg)
{
	for (;;) {
		long msg;
		struct go_case cs[] = { GO_RECV(inbox, &msg), GO_RECV(closed, NULL) };

		if (go_select(cs, 2, false) == 1)
			break;
		close_connection();	/* the model's ClusterConfig() callback */
	}
	go_close(dispatcher_loop_stopped);
	return NULL;
}

int main()
{
	go_init();
	inbox = go_make_chan(0);
	dispatcher_loop_stopped = go_make_chan(0);
	closed = go_make_chan(0);
	go(reader_loop, NULL);		/* c.Start() */
	go(dispatcher_loop, NULL);
	go_send(inbox, 1);		/* &ClusterConfig{} */
	go_recv(dispatcher_loop_stopped);
	return 0;
}
