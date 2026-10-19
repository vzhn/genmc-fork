/* GoBench cockroach#13197: a transaction that is never rolled back leaves its context watcher waiting */
#define BUG_NO_ROLLBACK
#include "../../../../correct/gobench/cockroach13197/cockroach13197.c"
