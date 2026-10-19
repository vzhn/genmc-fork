/* GoBench kubernetes#80284: two clients write onRotate without a lock */
#define BUG_UNLOCKED_ON_ROTATE
#include "../../../../correct/gobench/kubernetes80284/kubernetes80284.c"
