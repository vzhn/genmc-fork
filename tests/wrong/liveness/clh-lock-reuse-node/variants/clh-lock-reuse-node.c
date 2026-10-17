/* CLH unlock keeps its own node (TAMP Fig. 7.33): deadlock */
#define BUG_REUSE_NODE
#include "../../../../correct/classic/clh-lock/clh-lock.c"
