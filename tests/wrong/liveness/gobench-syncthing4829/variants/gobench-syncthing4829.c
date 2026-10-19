/* GoBench syncthing#4829: notify() read-locks the mapping clearAddresses() holds */
#define BUG_NOTIFY_UNDER_LOCK
#include "../../../../correct/gobench/syncthing4829/syncthing4829.c"
