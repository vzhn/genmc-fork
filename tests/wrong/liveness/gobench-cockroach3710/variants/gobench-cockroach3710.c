/* GoBench cockroach#3710: MaybeAdd() takes the read lock again under the scan's read lock */
#define BUG_MAYBE_ADD_UNDER_RLOCK
#include "../../../../correct/gobench/cockroach3710/cockroach3710.c"
