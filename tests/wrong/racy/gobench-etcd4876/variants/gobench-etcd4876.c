/* GoBench etcd#4876: the progress interval is written while a send loop reads it */
#define BUG_PLAIN_INTERVAL
#include "../../../../correct/gobench/etcd4876/etcd4876.c"
