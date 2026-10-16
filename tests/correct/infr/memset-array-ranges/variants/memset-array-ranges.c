#include <assert.h>
#include <string.h>

// %struct.pair = { i8, i32 }: 3 bytes of padding
struct pair {
	char c;
	int i;
};

int main()
{
	struct pair a[4];

	// every field of every element, padding skipped
	memset(a, 0x11, sizeof(a));
	assert(a[0].c == 0x11 && a[0].i == 0x11111111 && a[3].c == 0x11 && a[3].i == 0x11111111);

	// only the middle two elements
	memset(&a[1], 0, 2 * sizeof(a[0]));
	assert(a[0].c == 0x11 && a[0].i == 0x11111111 && a[1].c == 0 && a[1].i == 0 && a[2].c == 0 &&
	       a[2].i == 0 && a[3].c == 0x11 && a[3].i == 0x11111111);

	int m[3][4];

	// a whole 2D array, then one row of it
	memset(m, 0x22, sizeof(m));
	memset(m[1], 0, sizeof(m[1]));
	assert(m[0][0] == 0x22222222 && m[0][3] == 0x22222222 && m[1][0] == 0 && m[1][3] == 0 &&
	       m[2][0] == 0x22222222 && m[2][3] == 0x22222222);

	// the middle of a row
	memset(&m[2][1], 0, 2 * sizeof(int));
	assert(m[2][0] == 0x22222222 && m[2][1] == 0 && m[2][2] == 0 && m[2][3] == 0x22222222);

	// the tail of a row, reaching exactly the end of the array
	memset(&m[2][2], 0x33, 2 * sizeof(int));
	assert(m[2][1] == 0 && m[2][2] == 0x33333333 && m[2][3] == 0x33333333);
	return 0;
}
