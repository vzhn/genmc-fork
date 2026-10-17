/* synchronize_rcu() flips the phase once: a reader with a stale phase is skipped */
#define BUG_SINGLE_FLIP
#include "../../../../correct/classic/urcu/urcu.c"
