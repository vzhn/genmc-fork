/* GoBench cockroach#6181: String() takes the read lock again while a writer waits */
#define BUG_RECURSIVE_RLOCK
#include "../../../../correct/gobench/cockroach6181/cockroach6181.c"
