#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct rec {
	double d;
	float f;
	int i;
	bool b;
	const char *s;
};

struct rec g = { 1.5, 2.25f, -3, true, "hi" };

int main()
{
	// floating-point, bool and pointer fields survive a copy from a global...
	struct rec r = g;
	assert(r.d == 1.5 && r.f == 2.25f && r.i == -3 && r.b && r.s[0] == 'h');

	// ...and into untyped heap memory
	struct rec *h = malloc(sizeof(*h));
	*h = r;
	assert(h->d == 1.5 && h->f == 2.25f && h->i == -3 && h->b && h->s[1] == 'i');
	free(h);

	// memset must produce 0.0 / false / NULL for those field types
	memset(&r, 0, sizeof(r));
	assert(r.d == 0.0 && r.f == 0.0f && r.i == 0 && !r.b && r.s == NULL);
	return 0;
}
