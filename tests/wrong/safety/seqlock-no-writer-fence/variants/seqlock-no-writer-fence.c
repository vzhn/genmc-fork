/* Seqlock writer without the release fence (Boehm 2012): a torn read */
#define BUG_NO_WRITER_FENCE
#include "../../../../correct/data-structures/seqlock-boehm/seqlock-boehm.c"
