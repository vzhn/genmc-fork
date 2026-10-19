/* GoBench etcd#6873: stop() waits for the coalescer while holding its lock */
#define BUG_STOP_WAITS_UNDER_LOCK
#include "../../../../correct/gobench/etcd6873/etcd6873.c"
