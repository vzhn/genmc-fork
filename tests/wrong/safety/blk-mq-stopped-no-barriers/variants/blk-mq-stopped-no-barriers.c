/* BLK_MQ_S_STOPPED cleared and checked without barriers: the request is never dispatched */
#define BUG_NO_BARRIERS
#include "../../../../correct/classic/blk-mq-stopped/blk-mq-stopped.c"
