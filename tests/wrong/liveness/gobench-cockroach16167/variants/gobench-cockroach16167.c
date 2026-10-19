/* GoBench cockroach#16167: a read lock taken twice while a writer waits */
#define BUG_RECURSIVE_RLOCK
#include "../../../../correct/gobench/cockroach16167/cockroach16167.c"
