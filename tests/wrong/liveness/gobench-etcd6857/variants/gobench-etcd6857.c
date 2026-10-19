/* GoBench etcd#6857: Status() sends a request nobody will answer after Stop() */
#define BUG_STATUS_IGNORES_DONE
#include "../../../../correct/gobench/etcd6857/etcd6857.c"
