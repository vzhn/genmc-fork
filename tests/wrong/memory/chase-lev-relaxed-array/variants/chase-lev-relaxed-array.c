/* A resize publishes the new array with a relaxed store (Lê et al. 2013) */
#define GROW
#define BUG_RELAXED_ARRAY
#include "../../../../correct/data-structures/chase-lev-resize/chase-lev-resize.c"
