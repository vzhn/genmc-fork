#include "../../../lib/go.h"

/*
 * GoBench kernel grpc#2371 (non-blocking, nil pointer), after grpc-go's
 * ClientConn. The resolver's watcher goroutine hands service config
 * updates to the ClientConn, which, under cc.mu, passes the resolved
 * addresses to the balancer wrapper. Close() sets the balancer wrapper to
 * nil under the same mutex. The test dials and closes the connection at
 * once, so an update can arrive after Close(): a nil pointer dereference,
 * a panic in Go (an assertion here).
 *
 * BUG_NO_CLOSED_CHECK: handleServiceConfig() does not check whether the
 * connection is closed, as before grpc-go PR #2371 ("clientconn: fix race
 * when service config updated while closing", 2018).
 */

struct cc_balancer_wrapper {
	go_chan_t *resolver_update_ch;
};

struct client_conn {
	go_rwmutex_t mu;
	struct cc_balancer_wrapper *balancer_wrapper;
	bool closed;
};

struct client_conn cc;
struct cc_balancer_wrapper ccb;

static void handle_resolved_addrs(struct cc_balancer_wrapper *b)
{
	assert(b != NULL && "nil pointer dereference");
	struct go_case cs[] = { GO_RECV(b->resolver_update_ch, NULL) };
	go_select(cs, 1, true);
	go_send(b->resolver_update_ch, 1);
}

/* the resolver's watcher: cc.handleServiceConfig() */
void *watcher(void *arg)
{
	go_rwmutex_lock(&cc.mu);
#ifndef BUG_NO_CLOSED_CHECK
	if (cc.closed) {
		go_rwmutex_unlock(&cc.mu);
		return NULL;
	}
#endif
	handle_resolved_addrs(cc.balancer_wrapper);
	go_rwmutex_unlock(&cc.mu);
	return NULL;
}

void *close_conn(void *arg)
{
	go_rwmutex_lock(&cc.mu);
	cc.closed = true;
	cc.balancer_wrapper = NULL;
	go_rwmutex_unlock(&cc.mu);
	return NULL;
}

int main()
{
	go_init();
	ccb.resolver_update_ch = go_make_chan(1);	/* Dial() */
	cc.balancer_wrapper = &ccb;
	go(watcher, NULL);
	go(close_conn, NULL);
	return 0;
}
