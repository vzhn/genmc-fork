/* The grace period is of another srcu_struct than the reader's */
#define BUG_OTHER_SRCU
#include "../../../../correct/lkmm/srcu-sync-free/srcu-sync-free.c"
