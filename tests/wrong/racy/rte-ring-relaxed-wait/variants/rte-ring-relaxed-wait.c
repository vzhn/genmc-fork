/* update_tail() waits relaxed: no C11 sync with the earlier producer */
#define BUG_RELAXED_WAIT
#include "../../../../correct/data-structures/rte-ring/rte-ring.c"
