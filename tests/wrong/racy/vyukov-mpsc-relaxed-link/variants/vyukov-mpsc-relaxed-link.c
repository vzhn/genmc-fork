/* The producer links with a relaxed store: the payload read races */
#define BUG_RELAXED_LINK
#include "../../../../correct/data-structures/vyukov-mpsc/vyukov-mpsc.c"
