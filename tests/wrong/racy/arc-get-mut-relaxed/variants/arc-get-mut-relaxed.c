/* Rust Arc::get_mut before #51780: a relaxed load of the strong count */
#define BUG_RELAXED_STRONG
#include "../../../../correct/classic/arc-get-mut/arc-get-mut.c"
