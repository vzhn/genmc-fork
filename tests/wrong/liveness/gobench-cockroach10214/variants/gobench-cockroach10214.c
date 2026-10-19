/* GoBench cockroach#10214: coalescedMu held while taking raftMu */
#define BUG_SEND_HOLDS_COALESCED_MU
#include "../../../../correct/gobench/cockroach10214/cockroach10214.c"
