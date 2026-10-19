/* GoBench grpc#2371: a service config update after Close() uses the nil balancer wrapper */
#define BUG_NO_CLOSED_CHECK
#include "../../../../correct/gobench/grpc2371/grpc2371.c"
