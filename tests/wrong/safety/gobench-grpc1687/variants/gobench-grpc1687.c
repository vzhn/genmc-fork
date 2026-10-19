/* GoBench grpc#1687: a write after WriteStatus() sends on the closed writes channel */
#define BUG_WRITES_CLOSED_TRANSPORT_OPEN
#include "../../../../correct/gobench/grpc1687/grpc1687.c"
