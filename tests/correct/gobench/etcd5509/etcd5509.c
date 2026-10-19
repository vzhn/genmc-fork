#include "../../../lib/go.h"

/*
 * GoBench kernel etcd#5509 (blocking, double locking), after etcd's
 * clientv3. A KV request acquires the client's connection: it takes the
 * client's RWMutex for reading, sees whether the client is closed, and if
 * not, unlocks and waits for the connection. Close() takes the lock for
 * writing, cancels the client, briefly unlocks and locks again.
 *
 * When the client is already closed, acquire() returned ErrConnClosed
 * still holding the read lock, so Close() blocks retaking the write lock,
 * and the test, waiting for both, never finishes.
 *
 * Here a request that finds the client open waits until it is closed and
 * tries again.
 *
 * BUG_RETURN_HOLDING_RLOCK: acquire() returns without RUnlock(), as before
 * etcd PR #5509 ("clientv3: fix deadlock when closed", 2016).
 */

struct client {
	go_rwmutex_t mu;
	bool cancel;		/* the cancel func; false once closed */
	atomic_bool closed;	/* for the model's wait */
};

struct client cli = { .cancel = true };
go_mutex_t rc_mu;
go_chan_t *donec;

static void close_client(void)
{
	go_rwmutex_lock(&cli.mu);
	if (!cli.cancel) {
		go_rwmutex_unlock(&cli.mu);
		return;
	}
	cli.cancel = false;
	atomic_store(&cli.closed, true);
	go_rwmutex_unlock(&cli.mu);
	go_rwmutex_lock(&cli.mu);
	go_rwmutex_unlock(&cli.mu);	/* defer c.mu.Unlock() */
}

/* returns false for ErrConnClosed */
static bool acquire(void)
{
	for (;;) {
		go_rwmutex_rlock(&cli.mu);
		bool closed = !cli.cancel;
		go_mutex_lock(&rc_mu);
		go_mutex_unlock(&rc_mu);
		if (closed) {
#ifndef BUG_RETURN_HOLDING_RLOCK
			go_rwmutex_runlock(&cli.mu);
#endif
			return false;
		}
		go_rwmutex_runlock(&cli.mu);
		/* wait for the connection to change */
		__VERIFIER_await(atomic_load(&cli.closed));
	}
}

void *get(void *arg)
{
	acquire();		/* kv.Get() -> Do() -> do() -> getRemote() */
	go_close(donec);
	return NULL;
}

int main()
{
	go_init();
	donec = go_make_chan(0);
	go(get, NULL);
	close_client();
	go_recv(donec);
	return 0;
}
