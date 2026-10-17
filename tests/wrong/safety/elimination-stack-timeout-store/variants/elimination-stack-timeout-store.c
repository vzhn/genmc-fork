/* A waiter gives up with a store, not a CAS: its pushed value is popped twice */
#define BUG_TIMEOUT_STORE
#include "../../../../correct/data-structures/elimination-stack/elimination-stack.c"
