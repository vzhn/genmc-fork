#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// %struct.geom = { i32, i32, i32, i32, i32, i64 }
struct geom {
    uint32_t a, b, c, d, e;
    uint64_t f;
};

// The initializer makes the padding explicit, so the global gets a different type:
// @g = global { i32, i32, i32, i32, i32, [4 x i8], i64 }
static struct geom g = { .a = 3, .b = 2, .c = 8, .d = 1, .e = 2, .f = 1 };

struct params {
    struct geom geom;
    int n;
};

int main() {
    // memcpy from @g into a field of type %struct.geom
    struct params p = { .geom = g, .n = 2 };
    assert(p.geom.a == 3 && p.geom.e == 2 && p.geom.f == 1 && p.n == 2);

    // dst opaque (malloc), src typed
    struct geom *h = malloc(sizeof(*h));
    memcpy(h, &p.geom, sizeof(*h));
    assert(h->c == 8 && h->f == 1);
    free(h);
    return 0;
}
