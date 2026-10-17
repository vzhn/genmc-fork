/* tokio#8545: an unsigned compare frees a block once the tail index wraps */
#define BUG_WRAPPING_COMPARE
#include "../../../../correct/data-structures/tokio-mpsc/tokio-mpsc.c"
