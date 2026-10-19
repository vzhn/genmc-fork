#include "../../../lib/go.h"

/*
 * GoBench kernel kubernetes#82550 (non-blocking, data race), after the
 * kubelet's lazy ECR credential provider. LazyProvide() creates the actual
 * provider on first use; the kubelet may call it from two places at once
 * during startup (pulling a pod's image and the sandbox image).
 *
 * BUG_UNLOCKED_LAZY_INIT: the lazy initialisation is unsynchronised, as
 * before Kubernetes PR #82550 ("Fix racy lazyEcrProvider updates", 2019),
 * which takes a mutex around it.
 */

#define NCALLS 2

struct caching_provider {
	int cache;
};

struct lazy_ecr_provider {
	go_rwmutex_t mu;
	struct caching_provider *actual_provider;
};

struct lazy_ecr_provider provider;
struct caching_provider providers[NCALLS];
atomic_int nproviders;

void *lazy_provide(void *arg)
{
#ifndef BUG_UNLOCKED_LAZY_INIT
	go_rwmutex_lock(&provider.mu);
#endif
	if (!provider.actual_provider)
		provider.actual_provider = &providers[atomic_fetch_add(&nproviders, 1)];
	(void) provider.actual_provider->cache;		/* Provide()["0"] */
#ifndef BUG_UNLOCKED_LAZY_INIT
	go_rwmutex_unlock(&provider.mu);
#endif
	return NULL;
}

int main()
{
	go_init();
	for (int i = 0; i < NCALLS; i++)
		go(lazy_provide, NULL);
	return 0;
}
