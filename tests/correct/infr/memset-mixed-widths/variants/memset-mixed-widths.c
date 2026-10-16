#include <assert.h>
#include <stdint.h>
#include <string.h>

struct mixed {
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
	uint64_t u64;
	int32_t s32;
	int8_t s8;
};

int main()
{
	struct mixed m;

	// the fill byte is splatted to each field's width
	memset(&m, 0x5A, sizeof(m));
	assert(m.u8 == 0x5A && m.u16 == 0x5A5A && m.u32 == 0x5A5A5AU * 0x100 + 0x5A &&
	       m.u64 == 0x5A5A5A5A5A5A5A5AULL && m.s32 == 0x5A5A5A5A && m.s8 == 0x5A);

	// a byte with the top bit set gives all-ones, i.e. -1 for signed fields
	memset(&m, 0xFF, sizeof(m));
	assert(m.u8 == 0xFF && m.u16 == 0xFFFF && m.u32 == UINT32_MAX && m.u64 == UINT64_MAX &&
	       m.s32 == -1 && m.s8 == -1);

	// the fill value is an int; only its low byte counts
	memset(&m, 0x1234, sizeof(m));
	assert(m.u8 == 0x34 && m.u16 == 0x3434 && m.u64 == 0x3434343434343434ULL);
	return 0;
}
