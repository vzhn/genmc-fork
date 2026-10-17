/* glibc bug 13690: unlock reads the mutex after releasing it */
#define BUG_KIND_AFTER_RELEASE
#include "../../../../correct/classic/mutex-unlock-uaf/mutex-unlock-uaf.c"
