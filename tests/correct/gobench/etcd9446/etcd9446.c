#include "../../../lib/go.h"

/*
 * GoBench kernel etcd#9446 (non-blocking, data race), after etcd's
 * mvcc/backend. Read transactions range over the read buffer; a defrag
 * resets the read transaction, clearing that buffer. One goroutine resets
 * the transaction while another ranges over it.
 *
 * BUG_RESET_WITHOUT_LOCK: the reset does not take the read transaction's
 * lock, as before etcd commit 576a2ca501e9 ("mvcc/backend: defrag to block
 * concurrent read requests while resetting tx", 2018).
 */

#define NBUCKETS 2

struct read_tx {
	go_rwmutex_t mu;
	bool buckets[NBUCKETS];		/* map[string]struct{} */
};

struct read_tx txn;
go_waitgroup_t wg;

void *reset(void *arg)
{
#ifndef BUG_RESET_WITHOUT_LOCK
	go_rwmutex_lock(&txn.mu);
#endif
	for (int k = 0; k < NBUCKETS; k++)
		txn.buckets[k] = false;
#ifndef BUG_RESET_WITHOUT_LOCK
	go_rwmutex_unlock(&txn.mu);
#endif
	go_wg_done(&wg);
	return NULL;
}

void *unsafe_range(void *arg)
{
	go_rwmutex_rlock(&txn.mu);
	(void) txn.buckets[1];
	go_rwmutex_runlock(&txn.mu);
	go_wg_done(&wg);
	return NULL;
}

void *setup(void *arg)
{
	txn.buckets[1] = true;
	go(reset, NULL);
	go(unsafe_range, NULL);
	go_wg_done(&wg);
	return NULL;
}

int main()
{
	go_init();
	go_wg_add(&wg, 3);
	go(setup, NULL);
	go_wg_wait(&wg);
	return 0;
}
