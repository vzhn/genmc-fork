#define LOCK
#define AFTER_LOOP assert(0) /* reachable once thread_2 has run */
#include "../zne-loop-exit.c"
