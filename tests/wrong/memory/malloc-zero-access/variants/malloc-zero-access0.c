#include <stdlib.h>

/* malloc(0) returns a pointer that may not be dereferenced */

int main()
{
	char *p = malloc(0);

	*p = 1;
	free(p);
	return 0;
}
