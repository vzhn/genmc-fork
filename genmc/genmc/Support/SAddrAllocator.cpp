/*
 * GenMC -- Generic Model Checking.
 *
 * This project is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 *
 * Apache License 2.0:
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * MIT License:
 *     https://opensource.org/licenses/MIT
 */

#include "genmc/Support/SAddrAllocator.hpp"
#include "genmc/ADT/VectorClock.hpp"

#include <algorithm>

void SAddrAllocator::reserve(SAddr addr, uint64_t size)
{
	VERIFY(addr.isDynamic());
	auto tid = static_cast<int>((addr.get() & SAddr::threadMask) >> SAddr::threadStartBit);
	auto end = (addr.get() & SAddr::indexMask) + size;
	SAddr::Width &pool = dynamicPool_[tid];
	pool = std::max(pool, end);
}

auto SAddrAllocator::dynamicPoolEnd(int thread) const -> SAddr::Width
{
	auto it = dynamicPool_.find(thread);
	return it == dynamicPool_.end() ? 1 : static_cast<SAddr::Width>(it->second);
}

void SAddrAllocator::restrict(const VectorClock &view)
{
	for (auto &[tid, index] : dynamicPool_) {
		index = std::max(1, view.getMax(tid)); // don't allocate null
	}
}
