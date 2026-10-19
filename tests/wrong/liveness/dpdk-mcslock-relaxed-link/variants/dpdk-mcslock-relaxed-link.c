/* DPDK MCS lock links into the predecessor with a relaxed store: the waiter's locked = 1 lands after the hand-off */
#define BUG_RELAXED_LINK
#include "../../../../correct/classic/dpdk-mcslock/dpdk-mcslock.c"
