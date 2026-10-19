/* GoBench kubernetes#26980: pop() blocks on its channel holding the listener lock */
#define BUG_POP_BLOCKS_HOLDING_LOCK
#include "../../../../correct/gobench/kubernetes26980/kubernetes26980.c"
