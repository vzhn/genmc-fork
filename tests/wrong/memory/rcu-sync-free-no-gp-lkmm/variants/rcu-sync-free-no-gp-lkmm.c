/* The RCU callback frees the old node without waiting for a grace period */
#define BUG_NO_GP
#include "../../../../correct/lkmm/rcu-sync-free/rcu-sync-free.c"
