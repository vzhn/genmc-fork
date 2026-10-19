/* GoBench hugo#3251: the URL's mutex is taken under the map's write lock */
#define BUG_URL_LOCK_UNDER_MAP_LOCK
#include "../../../../correct/gobench/hugo3251/hugo3251.c"
