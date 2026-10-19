/* GoBench cockroach#10790: the goroutine waiting for prerequisites ignores shutdown */
#define BUG_DRAIN_IGNORES_QUIESCE
#include "../../../../correct/gobench/cockroach10790/cockroach10790.c"
