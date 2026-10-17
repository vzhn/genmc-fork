/* The dequeue loads the cell sequence relaxed: the data read races */
#define BUG_RELAXED_SEQ
#include "../../../../correct/data-structures/vyukov-queue/vyukov-queue.c"
