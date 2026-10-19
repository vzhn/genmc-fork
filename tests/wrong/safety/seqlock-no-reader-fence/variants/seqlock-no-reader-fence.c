/* Seqlock reader without the acquire fence (Boehm 2012): a torn read */
#define BUG_NO_READER_FENCE
#include "../../../../correct/data-structures/seqlock-boehm/seqlock-boehm.c"
