#include "../../../lib/go.h"

/*
 * GoBench kernel grpc#660 (blocking, channel), after grpc-go's benchmark
 * client. The close-loop client starts one RPC at a time: a goroutine runs
 * the call and reports on a fresh unbuffered done channel, while the loop
 * waits for done or for the stop channel to close. Shutdown closes stop.
 *
 * If stop wins, the loop returns and the RPC goroutine blocks forever
 * sending on done. The model runs up to NRPCS calls and then only waits
 * for stop.
 *
 * BUG_UNCONDITIONAL_DONE: the RPC goroutine sends on done unconditionally,
 * as before grpc-go commit ceacfbcbc151 ("Add selecting on stop chan to
 * avoid goroutine leak", 2016), which selects between done and stop.
 */

#ifndef NRPCS
# define NRPCS 2
#endif

go_chan_t *stop;

void *rpc(void *arg)
{
	go_chan_t *done = arg;

#ifdef BUG_UNCONDITIONAL_DONE
	go_send(done, 1);
#else
	struct go_case cs[] = { GO_RECV(stop, NULL), GO_SEND(done, 1) };
	go_select(cs, 2, false);
#endif
	return NULL;
}

/* G1: bc.doCloseLoopUnary() */
void *do_close_loop_unary(void *arg)
{
	for (int i = 0; i < NRPCS; i++) {
		go_chan_t *done = go_make_chan(0);

		go(rpc, done);
		struct go_case cs[] = { GO_RECV(stop, NULL), GO_RECV(done, NULL) };
		if (go_select(cs, 2, false) == 0)
			return NULL;
	}
	go_recv(stop);
	return NULL;
}

void *shutdown(void *arg)
{
	go_close(stop);
	return NULL;
}

int main()
{
	go_init();
	stop = go_make_chan(0);
	go(do_close_loop_unary, NULL);
	go(shutdown, NULL);
	return 0;
}
