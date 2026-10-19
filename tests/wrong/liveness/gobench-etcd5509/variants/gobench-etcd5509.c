/* GoBench etcd#5509: acquire() returns ErrConnClosed holding the read lock */
#define BUG_RETURN_HOLDING_RLOCK
#include "../../../../correct/gobench/etcd5509/etcd5509.c"
