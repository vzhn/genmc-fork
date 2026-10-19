/* urcu_synchronize() flips the phase once: a reader with a stale phase is skipped */
#define BUG_SINGLE_FLIP
#include "../../../../correct/lkmm/urcu-mb/urcu-mb.c"
