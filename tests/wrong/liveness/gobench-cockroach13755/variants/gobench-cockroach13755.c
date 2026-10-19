/* GoBench cockroach#13755: rows that are never closed leave their context watcher waiting */
#define BUG_ROWS_NOT_CLOSED
#include "../../../../correct/gobench/cockroach13755/cockroach13755.c"
