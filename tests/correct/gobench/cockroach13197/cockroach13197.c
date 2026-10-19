#include "../../../lib/go.h"

/*
 * GoBench kernel cockroach#13197 (blocking, context), after Go 1.8's
 * database/sql as used by CockroachDB's tests. Beginning a transaction
 * derives a cancellable context and starts a goroutine that waits for it
 * to be done; committing or rolling back cancels it. A test that begins a
 * transaction and never ends it leaves that goroutine waiting forever.
 *
 * BUG_NO_ROLLBACK: the transaction is never rolled back, as before
 * CockroachDB commit 9bf770cd8f6e ("sql: fix go1.8 goroutine leaks",
 * 2017).
 */

struct tx {
	go_ctx_t *ctx;
};

void *await_done(void *arg)
{
	struct tx *tx = arg;

	go_recv(go_ctx_done(tx->ctx));
	return NULL;
}

struct tx the_tx;

static struct tx *begin(go_ctx_t *ctx)
{
	struct tx *tx = &the_tx;

	tx->ctx = go_ctx_with_cancel(ctx);
	go(await_done, tx);
	return tx;
}

static void rollback(struct tx *tx)
{
	go_ctx_cancel(tx->ctx);
}

int main()
{
	go_init();
	struct tx *tx = begin(go_ctx_background());
#ifndef BUG_NO_ROLLBACK
	rollback(tx);
#else
	(void) rollback;
	(void) tx;
#endif
	return 0;
}
