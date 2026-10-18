/* futex_wake() without barrier (B), fixed by Linux 76835b0ebf8a: lost wakeup (kernel primitives) */
#define BUG_NO_MB
#include "../../../../correct/lkmm/futex-wake/futex-wake.c"
