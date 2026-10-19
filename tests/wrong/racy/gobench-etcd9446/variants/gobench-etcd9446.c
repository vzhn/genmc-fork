/* GoBench etcd#9446: resetting the read buffer races with a range */
#define BUG_RESET_WITHOUT_LOCK
#include "../../../../correct/gobench/etcd9446/etcd9446.c"
