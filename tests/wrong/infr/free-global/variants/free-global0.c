#include <stdlib.h>

/* free() of a global is an invalid free, not an internal failure */

int x;

int main()
{
	free(&x);
	return 0;
}
