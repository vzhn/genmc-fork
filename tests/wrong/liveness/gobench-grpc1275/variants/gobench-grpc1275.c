/* GoBench grpc#1275: CloseStream() leaves the stream's reader blocked */
#define BUG_CLOSE_WITHOUT_ERROR
#include "../../../../correct/gobench/grpc1275/grpc1275.c"
