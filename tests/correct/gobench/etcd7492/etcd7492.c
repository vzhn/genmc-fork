#include "../../../lib/go.h"

/*
 * GoBench kernel etcd#7492 (blocking, channel and lock), after etcd's
 * auth/simple_token.go. A keeper goroutine owns the simple tokens' TTLs:
 * new tokens reach it on addSimpleTokenCh (buffer of one), and on each tick
 * it expires tokens, calling back into the token store, which takes
 * simpleTokensMu. Assigning a token takes simpleTokensMu and, holding it,
 * sends the token to the keeper. NAUTH clients authenticate concurrently;
 * then the store is disabled, which stops the keeper.
 *
 * When the keeper's expiry callback waits for simpleTokensMu, a client
 * holding that lock can block sending on the full channel the keeper no
 * longer drains.
 *
 * BUG_TOKEN_CHANNEL: tokens reach the keeper through the channel, as before
 * etcd commit 1b1fabef8ffe ("auth: get rid of deadlocking channel passing
 * scheme in simpleTokenTTL", 2017), which guards the keeper's map with its
 * own mutex, always taken before simpleTokensMu.
 */

#ifndef NAUTH
# define NAUTH 3
#endif

struct keeper {
	int tokens;			/* map[string]time.Time: the number of tokens */
	go_mutex_t tokens_mu;		/* the fix */
	go_chan_t *add_simple_token_ch;
	go_chan_t *stop_ch;		/* chan chan struct{} */
	go_chan_t *ticker;
};

struct keeper keeper;
go_mutex_t simple_tokens_mu;
go_waitgroup_t wg;

static void delete_token_func(void)
{
	go_mutex_lock(&simple_tokens_mu);
	go_mutex_unlock(&simple_tokens_mu);
}

void *ticker(void *arg)
{
	go_send(keeper.ticker, 1);	/* one tick */
	return NULL;
}

/* G1: tm.run() */
void *run(void *arg)
{
	for (;;) {
		long wait_ch;
#ifdef BUG_TOKEN_CHANNEL
		struct go_case cs[] = { GO_RECV(keeper.add_simple_token_ch, NULL),
					GO_RECV(keeper.ticker, NULL),
					GO_RECV(keeper.stop_ch, &wait_ch) };
		int n = 3, base = 0;
#else
		struct go_case cs[] = { GO_RECV(keeper.ticker, NULL),
					GO_RECV(keeper.stop_ch, &wait_ch) };
		int n = 2, base = 1;
#endif
		switch (go_select(cs, n, false) + base) {
		case 0:
			keeper.tokens++;
			break;
		case 1:
#ifndef BUG_TOKEN_CHANNEL
			go_mutex_lock(&keeper.tokens_mu);
#endif
			while (keeper.tokens > 0) {
				delete_token_func();
				keeper.tokens--;
			}
#ifndef BUG_TOKEN_CHANNEL
			go_mutex_unlock(&keeper.tokens_mu);
#endif
			break;
		case 2:
			keeper.tokens = 0;
			go_send((go_chan_t *) wait_ch, 1);
			return NULL;
		}
	}
}

/* G2: as.Authenticate() -> assignSimpleTokenToUser() */
void *authenticate(void *arg)
{
#ifdef BUG_TOKEN_CHANNEL
	go_mutex_lock(&simple_tokens_mu);
	go_send(keeper.add_simple_token_ch, 1);
	go_mutex_unlock(&simple_tokens_mu);
#else
	go_mutex_lock(&keeper.tokens_mu);
	go_mutex_lock(&simple_tokens_mu);
	keeper.tokens++;
	go_mutex_unlock(&simple_tokens_mu);
	go_mutex_unlock(&keeper.tokens_mu);
#endif
	go_wg_done(&wg);
	return NULL;
}

/* tearDown(): disable() -> keeper.stop() */
static void disable(void)
{
	go_chan_t *wait_ch = go_make_chan(0);

	go_send(keeper.stop_ch, (long) wait_ch);
	go_recv(wait_ch);
	go_close(keeper.stop_ch);
	go_mutex_lock(&simple_tokens_mu);
	go_mutex_unlock(&simple_tokens_mu);
}

int main()
{
	go_init();
	keeper.add_simple_token_ch = go_make_chan(1);
	keeper.stop_ch = go_make_chan(0);
	keeper.ticker = go_make_chan(1);
	go(run, NULL);			/* enable() */
	go(ticker, NULL);

	go_wg_add(&wg, NAUTH);
	for (int i = 0; i < NAUTH; i++)
		go(authenticate, NULL);
	go_wg_wait(&wg);
	disable();
	return 0;
}
