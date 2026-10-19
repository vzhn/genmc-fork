/* GoBench cockroach#35501: goroutines read the range variable the loop overwrites */
#define BUG_SHARED_RANGE_VAR
#include "../../../../correct/gobench/cockroach35501/cockroach35501.c"
