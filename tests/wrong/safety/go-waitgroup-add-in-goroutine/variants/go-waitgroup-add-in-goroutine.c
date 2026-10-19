/* WaitGroup.Add inside the goroutine (staticcheck SA2000): Wait returns early */
#define BUG_ADD_IN_GOROUTINE
#include "../../../../correct/classic/go-waitgroup/go-waitgroup.c"
