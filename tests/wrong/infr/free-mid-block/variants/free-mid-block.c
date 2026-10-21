#include <stdlib.h>

/* free() of a pointer into a block (not to its start) is an invalid free */

int main()
{
	int *p = malloc(2 * sizeof(int));

	free(p + 1);
	return 0;
}
