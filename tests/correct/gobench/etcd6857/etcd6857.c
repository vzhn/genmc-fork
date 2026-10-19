#include "../../../lib/go.h"

/*
 * GoBench kernel etcd#6857 (blocking, channel), after etcd's raft.node. The
 * node's event loop, run(), answers status requests sent on n.status (each
 * request carries a reply channel) until a stop request arrives on n.stop,
 * when it closes n.done and returns. Stop() sends the stop request, unless
 * n.done is already closed, and waits for n.done.
 *
 * One goroutine runs the loop, one asks for the status, one stops the node.
 * If the stop wins, nobody answers the status request any more.
 *
 * BUG_STATUS_IGNORES_DONE: Status() sends its request unconditionally, as
 * before etcd commit 7afc490c9578 ("raft: return empty status if node is
 * stopped", 2016), which selects on n.done as well.
 */

struct node {
	go_chan_t *status;	/* chan chan Status */
	go_chan_t *stop;
	go_chan_t *done;
};

struct node n;

/* G1 */
void *run(void *arg)
{
	for (;;) {
		long c;
		struct go_case cs[] = { GO_RECV(n.status, &c), GO_RECV(n.stop, NULL) };

		switch (go_select(cs, 2, false)) {
		case 0:
			go_send((go_chan_t *) c, 1);	/* c <- Status{} */
			break;
		case 1:
			go_close(n.done);
			return NULL;
		}
	}
}

/* G2 */
void *status(void *arg)
{
	go_chan_t *c = go_make_chan(0);

#ifdef BUG_STATUS_IGNORES_DONE
	go_send(n.status, (long) c);
	go_recv(c);
#else
	struct go_case cs[] = { GO_SEND(n.status, (long) c), GO_RECV(n.done, NULL) };

	if (go_select(cs, 2, false) == 0)
		go_recv(c);
#endif
	return NULL;
}

/* G3 */
void *stop(void *arg)
{
	struct go_case cs[] = { GO_SEND(n.stop, 1), GO_RECV(n.done, NULL) };

	if (go_select(cs, 2, false) == 1)
		return NULL;
	go_recv(n.done);
	return NULL;
}

int main()
{
	go_init();
	n.status = go_make_chan(0);
	n.stop = go_make_chan(0);
	n.done = go_make_chan(0);
	go(run, NULL);
	go(status, NULL);
	go(stop, NULL);
	return 0;
}
