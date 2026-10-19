#include "../../../lib/go.h"

/*
 * GoBench kernel grpc#1424 (blocking, channel), after grpc-go's
 * DialContext(). With a balancer, dialing starts lbWatcher(), which handles
 * address updates from the balancer's Notify() channel and closes doneChan
 * after the first successful connection; the dialing goroutine waits for
 * doneChan. Here the balancer's channel is closed before it sends any
 * addresses, as when the dial times out.
 *
 * lbWatcher() then leaves its loop without closing doneChan, and the
 * dialing goroutine waits forever.
 *
 * BUG_DONE_NOT_CLOSED: lbWatcher() does not close doneChan when the
 * balancer's channel closes, as before grpc-go commit 64bd0b04a7bb ("Fix a
 * goroutine leak in DialContext", 2017), which closes it on return.
 */

go_chan_t *addr_ch;	/* the balancer's Notify() channel */
go_mutex_t ac_mu;

/* G3 */
void *lb_watcher(void *arg)
{
	go_chan_t *done_chan = arg;

	for (;;) {
		bool ok;

		go_recv2(addr_ch, &ok);
		if (!ok)
			break;
		go_mutex_lock(&ac_mu);	/* tear down the old connections */
		go_mutex_unlock(&ac_mu);
	}
#ifndef BUG_DONE_NOT_CLOSED
	go_close(done_chan);
#endif
	return NULL;
}

/* G2 */
void *dial(void *arg)
{
	go_chan_t *wait_c = arg;
	go_chan_t *done_chan = go_make_chan(0);

	go(lb_watcher, done_chan);
	go_recv(done_chan);
	go_close(wait_c);
	return NULL;
}

/* G1: DialContext() */
void *dial_context(void *arg)
{
	go_chan_t *wait_c = go_make_chan(1);

	go(dial, wait_c);
	go_close(addr_ch);
	return NULL;
}

int main()
{
	go_init();
	addr_ch = go_make_chan(0);
	go(dial_context, NULL);
	return 0;
}
