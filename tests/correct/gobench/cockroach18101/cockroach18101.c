#include "../../../lib/go.h"

/*
 * GoBench kernel cockroach#18101 (blocking, channel and context), after
 * CockroachDB's RESTORE (pkg/ccl/sqlccl/restore.go). A goroutine splits and
 * scatters the spans and sends each on the buffered readyForImportCh; the
 * restore loop receives them until the context is cancelled, then returns.
 * After that nobody receives, and the sender blocks once the buffer fills.
 * The model sends NSPANS spans into a buffer of one.
 *
 * BUG_SEND_IGNORES_CTX: splitAndScatter() sends without watching the
 * context, as before CockroachDB commit 822bd176cc72 ("sqlccl: add missing
 * ctx select in splitAndScatter", 2017).
 */

#ifndef NSPANS
# define NSPANS 3
#endif

go_ctx_t *ctx;
go_chan_t *ready_for_import_ch;

/* G2: splitAndScatter(), then close the channel */
void *split_and_scatter(void *arg)
{
	for (int i = 0; i < NSPANS; i++) {
#ifdef BUG_SEND_IGNORES_CTX
		go_send(ready_for_import_ch, i != 0);
#else
		struct go_case cs[] = { GO_RECV(go_ctx_done(ctx), NULL),
					GO_SEND(ready_for_import_ch, i != 0) };
		if (go_select(cs, 2, false) == 0)
			break;
#endif
	}
	go_close(ready_for_import_ch);
	return NULL;
}

/* G1 */
void *restore(void *arg)
{
	ready_for_import_ch = go_make_chan(1);
	go(split_and_scatter, NULL);
	for (;;) {
		bool ok;

		go_recv2(ready_for_import_ch, &ok);
		if (!ok)
			break;
		go_recv(go_ctx_done(ctx));
		return NULL;
	}
	return NULL;
}

void *cancel(void *arg)
{
	go_ctx_cancel(ctx);
	return NULL;
}

int main()
{
	go_init();
	ctx = go_ctx_with_cancel(go_ctx_background());
	go(restore, NULL);
	go(cancel, NULL);
	return 0;
}
