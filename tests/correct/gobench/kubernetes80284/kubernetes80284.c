#include "../../../lib/go.h"

/*
 * GoBench kernel kubernetes#80284 (non-blocking, data race), after
 * client-go's exec credential plugin. Every client built from the same
 * Authenticator calls UpdateTransportConfig(), which installs the new
 * dialer's CloseAll as the authenticator's onRotate callback. Two clients
 * are built concurrently.
 *
 * BUG_UNLOCKED_ON_ROTATE: onRotate is written without a lock, as before
 * Kubernetes commit 48ddf3be8778 ("Fix a racing issue in client-go
 * UpdateTransportConfig", 2019), which keeps a list of callbacks under a
 * mutex.
 */

#define NCLIENTS 2

struct authenticator {
	go_mutex_t mu;
	int on_rotate;			/* the callback; with the fix, the last of the list */
	int n_on_rotate;
};

struct authenticator a;
go_waitgroup_t wg;

static void update_transport_config(int dialer)
{
#ifdef BUG_UNLOCKED_ON_ROTATE
	a.on_rotate = dialer;
#else
	go_mutex_lock(&a.mu);
	a.on_rotate = dialer;
	a.n_on_rotate++;
	go_mutex_unlock(&a.mu);
#endif
}

void *client(void *arg)
{
	update_transport_config((int)(long) arg);
	go_wg_done(&wg);
	return NULL;
}

int main()
{
	go_init();
	go_wg_add(&wg, NCLIENTS);
	for (int i = 0; i < NCLIENTS; i++)
		go(client, (void *)(long)(i + 1));
	go_wg_wait(&wg);
	return 0;
}
