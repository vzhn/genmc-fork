/* The original popTask() halt check, with a halting task: a worker that saw
 * the halt takes another queued task (the other fixes are in) */
#define HALT
#define CHILDREN 3	/* one left queued after a worker sees the halt */
#define FIX_WAKEUP
#define FIX_WAIT
#include "../../../../correct/infr/pool-protocol/pool-model.c"
