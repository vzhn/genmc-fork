/* GoBench cockroach#18101: the span sender blocks after the restore loop gave up */
#define BUG_SEND_IGNORES_CTX
#include "../../../../correct/gobench/cockroach18101/cockroach18101.c"
