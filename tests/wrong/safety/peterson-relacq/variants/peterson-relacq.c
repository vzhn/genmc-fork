/* The Peterson lock with release/acquire accesses: both threads enter under RC11 and IMM */
#define BUG_RELACQ
#include "../../../../correct/classic/peterson/peterson.c"
