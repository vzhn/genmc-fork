#include "../../../lib/go.h"

/*
 * GoBench kernel cockroach#13755 (blocking, context), after Go 1.8's
 * database/sql as used by CockroachDB's tests. A query's Rows derive a
 * cancellable context and start a goroutine that waits for it to be done
 * and then closes the rows; Rows.Close() cancels it. A test that never
 * closes its rows leaves that goroutine waiting forever.
 *
 * BUG_ROWS_NOT_CLOSED: the rows are never closed, as before CockroachDB
 * commit ef906076adc1 ("sql: deflake TestSchemaChangeReverseMutations",
 * 2017).
 */

struct rows {
	go_ctx_t *ctx;
	bool closed;
};

struct rows rs;

void *await_done(void *arg)
{
	go_recv(go_ctx_done(rs.ctx));
	rs.closed = true;	/* rs.close(ctx.Err()) */
	return NULL;
}

int main()
{
	go_init();
	rs.ctx = go_ctx_with_cancel(go_ctx_background());	/* initContextClose() */
	go(await_done, NULL);
#ifndef BUG_ROWS_NOT_CLOSED
	go_ctx_cancel(rs.ctx);	/* rs.Close() */
#endif
	return 0;
}
