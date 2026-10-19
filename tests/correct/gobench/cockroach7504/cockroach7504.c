#include "../../../lib/go.h"

/*
 * GoBench kernel cockroach#7504 (blocking, AB-BA deadlock), after
 * CockroachDB's SQL lease manager (sql/lease.go). Two locks: each lease's
 * mutex (A) and the table-name cache's mutex (B).
 *
 * - AcquireByName() looks the lease up in the name cache: it takes B, then
 *   the lease's A to update its reference count.
 * - Release() takes the table state's mutex and the lease's A, drops the
 *   reference and removes the lease, which removes it from the name cache:
 *   B while still holding A.
 *
 * One goroutine acquires a lease by name while another releases it; the
 * test waits for both with a WaitGroup.
 *
 * BUG_RELEASE_HOLDS_LEASE_LOCK: Release() holds A while it takes B, as
 * before CockroachDB commit cab761b9f5ee ("sql: avoid deadlock in lease
 * acquisition", 2016), which drops A before removing the lease.
 */

struct lease_state {
	go_mutex_t mu;		/* A */
	int refcount;
};

struct lease_state lease = { .refcount = 1 };
go_mutex_t table_state_mu;
go_mutex_t name_cache_mu;	/* B */
bool in_name_cache = true;
go_waitgroup_t wg;

static void name_cache_remove(struct lease_state *s)
{
	go_mutex_lock(&name_cache_mu);
	in_name_cache = false;
	go_mutex_unlock(&name_cache_mu);
}

/* G1: mgr.AcquireByName() -> tableNameCache.get() */
void *acquire_by_name(void *arg)
{
	go_mutex_lock(&name_cache_mu);
	if (in_name_cache) {
		go_mutex_lock(&lease.mu);
		lease.refcount++;
		go_mutex_unlock(&lease.mu);
	}
	go_mutex_unlock(&name_cache_mu);
	go_wg_done(&wg);
	return NULL;
}

/* G2: mgr.Release() -> tableState.release() */
void *release(void *arg)
{
	go_mutex_lock(&table_state_mu);
	go_mutex_lock(&lease.mu);
	bool remove = --lease.refcount == 0;
#ifdef BUG_RELEASE_HOLDS_LEASE_LOCK
	if (remove)
		name_cache_remove(&lease);
	go_mutex_unlock(&lease.mu);
#else
	go_mutex_unlock(&lease.mu);
	if (remove)
		name_cache_remove(&lease);
#endif
	go_mutex_unlock(&table_state_mu);
	go_wg_done(&wg);
	return NULL;
}

int main()
{
	go_init();
	go_wg_add(&wg, 2);
	go(acquire_by_name, NULL);
	go(release, NULL);
	go_wg_wait(&wg);
	return 0;
}
