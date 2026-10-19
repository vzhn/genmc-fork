/* GoBench kubernetes#13135: startCaching() and event processing take the two locks in opposite orders */
#define BUG_CACHER_LOCK_IN_START
#include "../../../../correct/gobench/kubernetes13135/kubernetes13135.c"
