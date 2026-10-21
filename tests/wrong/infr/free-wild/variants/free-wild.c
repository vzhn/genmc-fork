#include <stdlib.h>

/* free() of an address outside any block is an invalid free, not an internal failure */

int main()
{
	free((void *) 0xdeadbeef);
	return 0;
}
