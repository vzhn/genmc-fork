/* The SRCU callback frees the old node without waiting for a grace period */
#define BUG_NO_GP
#include "../../../../correct/lkmm/srcu-sync-free/srcu-sync-free.c"
