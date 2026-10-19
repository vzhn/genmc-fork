#include "../../../lib/go.h"

/*
 * GoBench kernel grpc#1275 (blocking, channel), after grpc-go's HTTP/2
 * client transport. A stream's reader blocks until the transport puts a
 * message or an error into the stream's receive buffer. The test reads from
 * a stream in a goroutine, closes the stream, and waits for the reader or
 * a timer.
 *
 * CloseStream() removed the stream without telling its reader, so the
 * reader blocks forever.
 *
 * BUG_CLOSE_WITHOUT_ERROR: CloseStream() writes nothing to the stream, as
 * before grpc-go commit 0669f3f89e03 ("transport: fix error handling on
 * Stream deletion", 2017), which writes the error to it first.
 */

go_chan_t *recv;	/* the stream's receive buffer */
go_chan_t *donec;

/* G2: stream.Read() -> recvBufferReader.Read() */
void *reader(void *arg)
{
	go_recv(recv);
	go_close(donec);
	return NULL;
}

static void close_stream(void)
{
#ifndef BUG_CLOSE_WITHOUT_ERROR
	go_send(recv, 1);	/* s.write(recvMsg{err: err}) */
#endif
}

int main()
{
	go_init();
	recv = go_make_chan(1);
	donec = go_make_chan(0);
	go(reader, NULL);
	close_stream();

	struct go_case cs[] = { GO_RECV(donec, NULL), GO_RECV(go_time_after(), NULL) };
	go_select(cs, 2, false);
	return 0;
}
