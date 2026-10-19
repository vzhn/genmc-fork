/* do_umount() checks the refcount without smp_mb(): a legitimized reference to an unmounted mount */
#define BUG_NO_UMOUNT_MB
#include "../../../../correct/classic/mnt-umount-busy/mnt-umount-busy.c"
