#include "../../../lib/go.h"

/*
 * GoBench kernel etcd#3077 (non-blocking, misused channel), after etcd's
 * EtcdServer and its raftNode. The server starts the raft node's goroutine
 * and, on shutdown, tells it to stop through r.stopped and waits for
 * r.done. The raft goroutine created those two channels itself when it
 * started, so the server reads the channel variables while the raft
 * goroutine may still be writing them.
 *
 * The test starts the server and stops it.
 *
 * BUG_RAFT_MAKES_ITS_CHANNELS: raftNode.run() creates r.stopped and r.done,
 * as before etcd PR #3077 (2015), which creates them in EtcdServer.run()
 * before starting the raft goroutine.
 */

struct raft_node {
	go_chan_t *stopped;
	go_chan_t *done;
};

struct etcd_server {
	struct raft_node r;
	go_chan_t *done;
	go_chan_t *stop;
};

struct etcd_server srv;

void *raft_run(void *arg)
{
#ifdef BUG_RAFT_MAKES_ITS_CHANNELS
	srv.r.stopped = go_make_chan(0);
	srv.r.done = go_make_chan(0);
#endif
	go_recv(srv.r.stopped);
	go_close(srv.r.done);		/* defer r.stop() */
	return NULL;
}

void *server_run(void *arg)
{
#ifndef BUG_RAFT_MAKES_ITS_CHANNELS
	srv.r.stopped = go_make_chan(0);
	srv.r.done = go_make_chan(0);
#endif
	go(raft_run, NULL);
	go_recv(srv.stop);
	go_send(srv.r.stopped, 1);	/* the deferred shutdown */
	go_recv(srv.r.done);
	go_close(srv.done);
	return NULL;
}

static void server_stop(void)
{
	struct go_case cs[] = { GO_SEND(srv.stop, 1), GO_RECV(srv.done, NULL) };

	if (go_select(cs, 2, false) == 1)
		return;
	go_recv(srv.done);
}

int main()
{
	go_init();
	srv.done = go_make_chan(0);	/* srv.start() */
	srv.stop = go_make_chan(0);
	go(server_run, NULL);
	server_stop();			/* defer srv.Stop() */
	return 0;
}
