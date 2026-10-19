/* tokio before a7a79f28a8fe: a relaxed CAS in drop_join_handle_fast() */
#define BUG_RELAXED_FAST_DROP
#include "../../../../correct/classic/tokio-join-handle/tokio-join-handle.c"
