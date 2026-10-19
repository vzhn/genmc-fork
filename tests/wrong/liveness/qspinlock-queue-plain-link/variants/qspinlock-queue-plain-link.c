/* qspinlock links into the predecessor with WRITE_ONCE(): the node's locked = 0 lands after the hand-off */
#define BUG_PLAIN_LINK
#include "../../../../correct/classic/qspinlock-queue/qspinlock-queue.c"
