/* GoBench moby#18412: the exit code is written while the timed-out caller reads it */
#define BUG_SHARED_EXIT_CODE
#include "../../../../correct/gobench/moby18412/moby18412.c"
