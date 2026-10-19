/* Rust std kmc-solid before #105590: joining publishes the task ID with acquire only */
#define BUG_JOIN_ACQUIRE
#include "../../../../correct/classic/itron-thread-lifecycle/itron-thread-lifecycle.c"
