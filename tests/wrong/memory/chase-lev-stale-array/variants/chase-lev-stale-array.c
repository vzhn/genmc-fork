/* crossbeam-deque CVE-2021-32810: a batch steal takes a task from a replaced array */
#define BUG_STALE_ARRAY
#include "../../../../correct/data-structures/chase-lev-resize/chase-lev-resize.c"
