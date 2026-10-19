/* MCS lock marks itself locked after linking: spins forever */
#define BUG_LOCKED_AFTER_LINK
#include "../../../../correct/classic/mcs-lock/mcs-lock.c"
