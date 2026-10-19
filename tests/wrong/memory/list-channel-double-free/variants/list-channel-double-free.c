/* crossbeam-channel RUSTSEC-2025-0024: a block is freed twice */
#define BUG_LOAD_IN_LOOP
#include "../../../../correct/data-structures/list-channel/list-channel.c"
