/*
 * The constant initializers of these variables do not have their declared
 * LLVM types: clang makes the padding after the lock an explicit field,
 * turns the array into a packed struct, and splits the bitfields.
 * The names printed for their fields should still follow the declaration.
 */

struct s {
	pthread_mutex_t lock;
	long a;
	long b;
};

struct s v = {.a = 1, .b = 2};

struct s arr[3] = {[1] = {.a = 3}, [2] = {.b = 4}};

struct outer {
	char c;
	struct s in;
} o = {.in = {.b = 5}};

struct bits {
	int x : 3;
	int y : 13;
	long c;
} bf = {.y = 6, .c = 7};

void *thread_1(void *unused)
{
	if (v.a == 1 && v.b == 2 && arr[1].a == 3 && arr[2].b == 4 && o.in.b == 5 && bf.c == 7)
		assert(0);
	return NULL;
}
