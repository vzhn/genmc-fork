#include "../../../lib/go.h"

/*
 * GoBench kernel moby#29733 (blocking, condition variable), after Docker's
 * plugin registry (pkg/plugins). waitActive() waits on the plugin's
 * activateWait condition until the plugin is activated. Registering a new
 * extension-point handler, Handle(), marked every plugin as not activated,
 * so that it would be activated again for the new handler; but nothing
 * activates an already running plugin again, so a later waitActive() waits
 * forever.
 *
 * The test waits for an activated plugin, registers a handler and waits
 * again; each wait runs in its own goroutine.
 *
 * BUG_HANDLE_DEACTIVATES: Handle() resets the plugins' activated flag, as
 * before moby PR #29733 ("Fix deadlock in handlers", 2017), which derives
 * activation from the plugin's manifest and leaves it alone.
 */

struct plugin {
	bool activated;
	go_mutex_t mu;
	go_cond_t activate_wait;
};

struct plugin p = { .activated = true };
go_rwmutex_t handlers;

static void wait_active(struct plugin *pl)
{
	go_mutex_lock(&pl->mu);
	while (!pl->activated)
		go_cond_wait(&pl->activate_wait, &pl->mu);
	go_mutex_unlock(&pl->mu);
}

static void handle(void)
{
	go_rwmutex_lock(&handlers);
#ifdef BUG_HANDLE_DEACTIVATES
	p.activated = false;
#endif
	go_rwmutex_unlock(&handlers);
}

void *waiter(void *arg)
{
	wait_active(&p);
	go_close((go_chan_t *) arg);
	return NULL;
}

static void test_active(void)
{
	go_chan_t *done = go_make_chan(0);

	go(waiter, done);
	go_recv(done);
}

int main()
{
	go_init();
	test_active();
	handle();
	test_active();
	return 0;
}
