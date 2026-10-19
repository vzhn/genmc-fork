/* GoBench cockroach#7504: Release() takes the name cache lock holding the lease lock */
#define BUG_RELEASE_HOLDS_LEASE_LOCK
#include "../../../../correct/gobench/cockroach7504/cockroach7504.c"
