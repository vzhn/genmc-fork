#include <assert.h>
#include <stdint.h>
#include <string.h>

struct quad {
    uint32_t a, b, c, d;
};

int main() {
    // clang zeroes the tail with memset(getelementptr i8, ptr %a, i64 12), 28 bytes
    int a[10] = { 'x', 'x', 'x' };
    assert(a[2] == 'x' && a[3] == 0 && a[9] == 0);

    // A non-zero byte fills every byte of wider fields
    struct quad q;
    memset(&q, 0x11, sizeof(q));
    assert(q.a == 0x11111111 && q.d == 0x11111111);

    // Only the given length is set, starting at a field
    q.c = 5;
    memset(&q.a, 0, 8);
    assert(q.a == 0 && q.b == 0 && q.c == 5 && q.d == 0x11111111);
    return 0;
}
