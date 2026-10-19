/* Ported MCS lock without an acquire barrier after the spin: the critical sections race */
#define BUG_NO_ACQUIRE
#include "../../../../correct/classic/mcs-lock-arm-port/mcs-lock-arm-port.c"
