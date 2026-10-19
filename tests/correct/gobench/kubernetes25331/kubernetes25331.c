#include "../../../lib/go.h"

/*
 * GoBench kernel kubernetes#25331 (blocking, channel and context), after
 * the etcd3 storage watcher (pkg/storage/etcd3/watcher.go). The watch
 * goroutine waits for an error or for its context to be cancelled; on an
 * error it cancels the context and sends the error on the unbuffered
 * resultChan. Stop() reports an error and cancels the context; nobody reads
 * the results any more.
 *
 * The watch goroutine then blocks forever sending the error result.
 *
 * BUG_RESULT_SEND_IGNORES_CTX: the error result is sent unconditionally
 * (after cancelling the context itself), as before Kubernetes commit
 * 97f4647dc3d8 ("etcd3/watcher: fix goroutine leak if ctx is canceled",
 * 2016), which selects between the send and the context being done, and
 * cancels afterwards.
 */

struct watch_chan {
	go_ctx_t *ctx;
	go_chan_t *result_chan;
	go_chan_t *err_chan;
};

struct watch_chan wc;

/* G1 */
void *run(void *arg)
{
	struct go_case cs[] = { GO_RECV(wc.err_chan, NULL), GO_RECV(go_ctx_done(wc.ctx), NULL) };

	if (go_select(cs, 2, false) == 0) {
#ifdef BUG_RESULT_SEND_IGNORES_CTX
		go_ctx_cancel(wc.ctx);
		go_send(wc.result_chan, 1);
#else
		struct go_case send[] = { GO_SEND(wc.result_chan, 1),
					  GO_RECV(go_ctx_done(wc.ctx), NULL) };
		go_select(send, 2, false);
		go_ctx_cancel(wc.ctx);
#endif
	}
	return NULL;
}

/* G2 */
void *stop(void *arg)
{
	go_send(wc.err_chan, 1);
	go_ctx_cancel(wc.ctx);
	return NULL;
}

int main()
{
	go_init();
	wc.ctx = go_ctx_with_cancel(go_ctx_background());
	wc.result_chan = go_make_chan(0);
	wc.err_chan = go_make_chan(0);
	go(run, NULL);
	go(stop, NULL);
	return 0;
}
