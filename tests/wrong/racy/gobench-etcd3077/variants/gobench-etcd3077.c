/* GoBench etcd#3077: the raft goroutine creates channels the server reads */
#define BUG_RAFT_MAKES_ITS_CHANNELS
#include "../../../../correct/gobench/etcd3077/etcd3077.c"
