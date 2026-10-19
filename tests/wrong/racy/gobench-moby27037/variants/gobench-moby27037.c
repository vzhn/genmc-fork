/* GoBench moby#27037: goroutine closures read the loop variable the loop increments */
#define BUG_SHARED_LOOP_VAR
#include "../../../../correct/gobench/moby27037/moby27037.c"
