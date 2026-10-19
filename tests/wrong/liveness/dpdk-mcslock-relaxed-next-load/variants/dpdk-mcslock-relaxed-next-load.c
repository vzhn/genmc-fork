/* DPDK MCS unlock reads the successor pointer relaxed: the waiter's locked = 1 lands after the hand-off */
#define BUG_RELAXED_NEXT_LOAD
#include "../../../../correct/classic/dpdk-mcslock/dpdk-mcslock.c"
