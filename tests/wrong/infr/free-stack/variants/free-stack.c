#include <stdlib.h>

/* free() of a stack variable is an invalid free (reported at the free,
 * not as a double free when the variable goes out of scope) */

int main()
{
	int x = 0;

	free(&x);
	return 0;
}
