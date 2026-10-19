/* GoBench kubernetes#25331: the watcher blocks sending a result nobody reads after Stop() */
#define BUG_RESULT_SEND_IGNORES_CTX
#include "../../../../correct/gobench/kubernetes25331/kubernetes25331.c"
