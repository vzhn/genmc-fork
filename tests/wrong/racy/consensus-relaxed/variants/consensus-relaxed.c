/* A relaxed CAS: a losing thread reads the winner's proposal without synchronizing */
#define BUG_RELAXED
#include "../../../../correct/classic/consensus/consensus.c"
