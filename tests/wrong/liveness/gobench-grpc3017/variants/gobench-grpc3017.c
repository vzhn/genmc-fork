/* GoBench grpc#3017: the cancelled timer's callback returns holding the mutex */
#define BUG_ABORT_WITHOUT_UNLOCK
#include "../../../../correct/gobench/grpc3017/grpc3017.c"
