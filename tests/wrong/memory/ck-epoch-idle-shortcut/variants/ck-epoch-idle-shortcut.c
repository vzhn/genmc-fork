/* ck_epoch before 0.6.0: synchronize stops early while a reader is active */
#define BUG_IDLE_SHORTCUT
#include "../../../../correct/data-structures/ck-epoch/ck-epoch.c"
