#include <stdlib.h>

/* Neither may the pointer calloc(0, n) returns, although calloc() zeroes the block */

int main()
{
	int *p = calloc(0, sizeof(int));
	int x = *p;

	free(p);
	return x;
}
