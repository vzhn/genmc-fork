/* DPDK rte_ring 2018-2025: a relaxed head load sees a stale tail and underflows */
#define BUG_PARTIAL_ORDER
#include "../../../../correct/data-structures/rte-ring/rte-ring.c"
