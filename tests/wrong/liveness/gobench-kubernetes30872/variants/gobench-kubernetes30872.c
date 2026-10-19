/* GoBench kubernetes#30872: ClustersSynced() and the controller loop take the two locks in opposite orders */
#define BUG_SYNCED_UNDER_INFORMER_LOCK
#include "../../../../correct/gobench/kubernetes30872/kubernetes30872.c"
