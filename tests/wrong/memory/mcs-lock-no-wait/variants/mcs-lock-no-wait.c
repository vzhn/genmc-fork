/* MCS unlock does not wait for the successor link: null dereference */
#define BUG_NO_WAIT
#include "../../../../correct/classic/mcs-lock/mcs-lock.c"
