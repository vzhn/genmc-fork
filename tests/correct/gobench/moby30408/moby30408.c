#include "../../../lib/go.h"

/*
 * GoBench kernel moby#30408 (blocking, condition variable), after Docker's
 * plugin registry (pkg/plugins). waitActive() waits on the plugin's
 * activateWait condition until the plugin is activated, which is when it
 * has a manifest. A plugin whose activation failed never gets one, so
 * waiting for it never ends.
 *
 * The test waits, in a goroutine, for a plugin whose activation failed.
 *
 * BUG_IGNORE_ACTIVATE_ERR: waitActive() waits only for the manifest, as
 * before moby PR #30408 (2017), which also stops waiting once activation
 * has failed.
 */

struct plugin {
	bool has_manifest;
	bool activate_err;
	go_mutex_t mu;
	go_cond_t activate_wait;
};

struct plugin p = { .activate_err = true };

static bool wait_active(struct plugin *pl)
{
	go_mutex_lock(&pl->mu);
#ifdef BUG_IGNORE_ACTIVATE_ERR
	while (!pl->has_manifest)
#else
	while (!pl->has_manifest && !pl->activate_err)
#endif
		go_cond_wait(&pl->activate_wait, &pl->mu);
	go_mutex_unlock(&pl->mu);
	return pl->activate_err;
}

void *waiter(void *arg)
{
	wait_active(&p);
	go_close((go_chan_t *) arg);
	return NULL;
}

int main()
{
	go_init();
	go_chan_t *done = go_make_chan(0);
	go(waiter, done);
	go_recv(done);
	return 0;
}
