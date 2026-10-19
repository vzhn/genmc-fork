#include "../../../lib/go.h"

/*
 * GoBench kernel syncthing#4829 (blocking, double locking), after
 * Syncthing's NAT mapping (lib/nat/structs.go). clearAddresses() holds the
 * mapping's RWMutex for writing while it removes the external addresses and
 * notifies the subscribers, and notify() takes the same lock for reading.
 * Removing a mapping that has an address clears it.
 *
 * BUG_NOTIFY_UNDER_LOCK: clearAddresses() notifies while holding the lock,
 * as before Syncthing PR #4829 (2018), which unlocks first.
 */

struct mapping {
	go_rwmutex_t mut;
	int ext_addresses;
};

struct mapping m = { .ext_addresses = 1 };
go_rwmutex_t service_mut;

static void notify(void)
{
	go_rwmutex_rlock(&m.mut);
	go_rwmutex_runlock(&m.mut);
}

static void clear_addresses(void)
{
	go_rwmutex_lock(&m.mut);
	int removed = m.ext_addresses;
	m.ext_addresses = 0;
#ifdef BUG_NOTIFY_UNDER_LOCK
	if (removed > 0)
		notify();
	go_rwmutex_unlock(&m.mut);
#else
	go_rwmutex_unlock(&m.mut);
	if (removed > 0)
		notify();
#endif
}

int main()
{
	go_init();
	go_rwmutex_lock(&service_mut);	/* natSvc.RemoveMapping(m) */
	clear_addresses();
	go_rwmutex_unlock(&service_mut);
	return 0;
}
