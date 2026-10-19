/* GoBench kubernetes#6632: a write blocked on resetChan holds the lock monitor() needs */
#define BUG_NO_DRAIN
#include "../../../../correct/gobench/kubernetes6632/kubernetes6632.c"
