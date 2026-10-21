#include <pthread.h>
#include <atomic>
#include <cassert>

/*
 * The type info of a class with virtual methods refers to the vtable of
 * __cxxabiv1::__class_type_info, which the module only declares, as an
 * `external global [0 x ptr]`: a zero-sized global without an initializer.
 */

struct D {
	std::atomic<long> d;
	virtual long get() { return d.load(); }
};

D dd;

void *thread_1(void *unused)
{
	dd.d.store(1);
	return nullptr;
}

int main()
{
	pthread_t t;

	pthread_create(&t, nullptr, thread_1, nullptr);

	long r = dd.get();
	assert(r == 0 || r == 1);

	pthread_join(t, nullptr);
	return 0;
}
