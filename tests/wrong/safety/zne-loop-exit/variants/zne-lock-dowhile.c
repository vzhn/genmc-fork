#define LOCK
#define DO_WHILE
#define AFTER_LOOP assert(0) /* reachable once thread_2 has run */
#include "../zne-loop-exit.c"
