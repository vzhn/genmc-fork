#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// %struct.inner = { i16, i64, i8 }: padding after a and after c
struct inner {
	uint16_t a;
	uint64_t b;
	uint8_t c;
};

struct outer {
	uint8_t tag;
	struct inner in;
	uint32_t tail;
};

struct inner src_in = { .a = 7, .b = 0x1122334455667788ULL, .c = 9 };

int main()
{
	// a sub-struct at a non-zero offset, on either side of the copy
	struct outer o = { .tag = 1, .tail = 4 };
	struct outer p = { .tag = 2, .tail = 5 };
	o.in = src_in;
	p.in = o.in;
	assert(o.tag == 1 && o.tail == 4 && o.in.a == 7 && o.in.b == 0x1122334455667788ULL &&
	       o.in.c == 9);
	assert(p.tag == 2 && p.tail == 5 && p.in.a == 7 && p.in.b == 0x1122334455667788ULL &&
	       p.in.c == 9);

	// an element of an array of structs, as destination and as source
	struct inner arr[3] = { 0 };
	arr[2] = src_in;
	arr[0] = arr[2];
	assert(arr[0].a == 7 && arr[0].c == 9 && arr[1].a == 0 && arr[1].b == 0 && arr[2].b == 0x1122334455667788ULL);

	// an element of a heap array: the pointer is a byte GEP off malloc(), so the
	// type must come from the other operand
	struct inner *h = malloc(4 * sizeof(*h));
	h[3] = src_in;
	assert(h[3].a == 7 && h[3].b == 0x1122334455667788ULL && h[3].c == 9);
	struct inner back = h[3];
	assert(back.a == 7 && back.c == 9);
	free(h);

	// a row of a 2D array
	int m[3][4] = { { 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { 9, 10, 11, 12 } };
	memcpy(m[0], m[2], sizeof(m[0]));
	assert(m[0][0] == 9 && m[0][3] == 12 && m[1][0] == 5 && m[1][3] == 8 && m[2][0] == 9);
	return 0;
}
