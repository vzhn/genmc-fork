/* GoBench etcd#7902: a follower waits for the leader holding the lock the leader needs */
#define BUG_RELEASE_UNDER_MUTEX
#include "../../../../correct/gobench/etcd7902/etcd7902.c"
