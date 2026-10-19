/* GoBench cockroach#2448: the state goroutine and the store block sending to each other */
#define BUG_EVENT_SEND_OUTSIDE_SELECT
#include "../../../../correct/gobench/cockroach2448/cockroach2448.c"
