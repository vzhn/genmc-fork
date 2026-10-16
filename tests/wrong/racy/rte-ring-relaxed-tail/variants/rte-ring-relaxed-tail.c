/* DPDK rte_ring C11 port before 2018: the other side's tail is a plain load */
#define BUG_RELAXED_TAIL
#include "../../../../correct/data-structures/rte-ring/rte-ring.c"
