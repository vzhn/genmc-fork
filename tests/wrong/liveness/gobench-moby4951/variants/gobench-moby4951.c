/* GoBench moby#4951: the global lock is dropped and retaken under a device lock */
#define BUG_GLOBAL_LOCK_FIRST
#include "../../../../correct/gobench/moby4951/moby4951.c"
