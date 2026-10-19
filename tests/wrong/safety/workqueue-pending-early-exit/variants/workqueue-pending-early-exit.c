/* test_and_set_bit() returns early without ordering: the queued work misses the new data */
#define BUG_EARLY_EXIT
#include "../../../../correct/classic/workqueue-pending/workqueue-pending.c"
