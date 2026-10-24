/*
 * exit() in main runs the atexit() handlers: the one here fails.
 */
#include <assert.h>
#include <stdlib.h>

void handler(void)
{
	assert(0 && "the handler runs");
}

int main()
{
	atexit(handler);
	exit(1);
}
