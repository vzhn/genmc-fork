/* NUMA rwlock read unlock with a relaxed decrement: the writer's update races with the reader's load */
#define BUG_RELAXED_READ_UNLOCK
#include "../../../../correct/classic/numa-rwlock/numa-rwlock.c"
