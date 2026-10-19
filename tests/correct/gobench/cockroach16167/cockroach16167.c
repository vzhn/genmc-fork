#include "../../../lib/go.h"

/*
 * GoBench kernel cockroach#16167 (blocking, recursive read lock), after
 * CockroachDB's sql.Executor. The executor's system config is guarded by an
 * RWMutex; a statement runs holding it for reading (through a sync.Cond
 * whose Locker is the RWMutex's RLocker) and, deeper in the call chain,
 * reads the database cache, taking the read lock again. A gossip update
 * takes the write lock.
 *
 * Go's RWMutex is not reentrant for readers: once a writer waits, new
 * readers block. If the update's Lock() comes between the statement's two
 * RLock()s, the statement waits for the writer and the writer waits for the
 * statement.
 *
 * BUG_RECURSIVE_RLOCK: the recursive read lock, as before CockroachDB commit
 * d064942b067a ("sql: avoid recursive acquisition of
 * Executor.systemConfigMu", 2017), which made the lock a plain mutex and
 * the cache an atomic value read without it.
 */

#ifdef BUG_RECURSIVE_RLOCK
go_rwmutex_t system_config_mu;
#else
go_mutex_t system_config_mu;
atomic_int database_cache;
#endif

static void get_database_cache(void)
{
#ifdef BUG_RECURSIVE_RLOCK
	go_rwmutex_rlock(&system_config_mu);
	go_rwmutex_runlock(&system_config_mu);
#else
	(void) atomic_load(&database_cache);
#endif
}

/* G1: e.Start() -> updateSystemConfig() */
void *start(void *arg)
{
#ifdef BUG_RECURSIVE_RLOCK
	go_rwmutex_lock(&system_config_mu);
	go_rwmutex_unlock(&system_config_mu);
#else
	go_mutex_lock(&system_config_mu);
	atomic_store(&database_cache, 1);
	go_mutex_unlock(&system_config_mu);
#endif
	return NULL;
}

/* G2: e.execParsed() -> ... -> Prepare() -> getDatabaseCache() */
static void exec_parsed(void)
{
#ifdef BUG_RECURSIVE_RLOCK
	go_rwmutex_rlock(&system_config_mu);	/* systemConfigCond.L.Lock() */
	get_database_cache();
	go_rwmutex_runlock(&system_config_mu);
#else
	go_mutex_lock(&system_config_mu);
	go_mutex_unlock(&system_config_mu);
	get_database_cache();
#endif
}

int main()
{
	go_init();
	go(start, NULL);
	exec_parsed();
	return 0;
}
