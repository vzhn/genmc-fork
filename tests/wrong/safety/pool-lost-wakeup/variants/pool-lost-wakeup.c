/* The original popTask() wait: a worker can go to sleep while a task is
 * queued (the other fixes are in, so only this property can fail) */
#define FIX_HALT
#define FIX_WAIT
#include "../../../../correct/infr/pool-protocol/pool-model.c"
