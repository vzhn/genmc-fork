#include "../../../lib/go.h"

/*
 * GoBench kernel moby#27037 (non-blocking, anonymous function), after a
 * Docker integration test. A loop starts one goroutine per API version; each
 * goroutine is a closure that formats the version from the loop variable.
 * Before Go 1.22 the closures share the single loop variable, which the
 * loop keeps incrementing while the goroutines read it.
 *
 * BUG_SHARED_LOOP_VAR: the closures capture the loop variable, as before
 * moby commit ba9fb732801c ("Fix datarace in
 * ApiStatsNetworkStatsVersioning", 2016), which passes it as an argument.
 */

#define FIRST 17
#define LAST 19

go_waitgroup_t wg;
int i;			/* the loop variable the closures capture */

void *closure(void *arg)
{
#ifdef BUG_SHARED_LOOP_VAR
	int v = i;
#else
	int v = (int)(long) arg;
#endif
	assert(v >= FIRST && v <= LAST + 1);	/* fmt.Sprintf("v1.%d", i) */
	go_wg_done(&wg);
	return NULL;
}

int main()
{
	go_init();
	for (i = FIRST; i <= LAST; i++) {
		go_wg_add(&wg, 1);
		go(closure, (void *)(long) i);
	}
	go_wg_wait(&wg);
	return 0;
}
