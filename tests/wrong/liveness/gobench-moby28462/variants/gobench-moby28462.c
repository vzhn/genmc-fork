/* GoBench moby#28462: the stop request is sent under the lock the monitor waits for */
#define BUG_SEND_STOP
#include "../../../../correct/gobench/moby28462/moby28462.c"
