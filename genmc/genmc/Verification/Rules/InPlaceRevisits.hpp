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


#ifndef GENMC_IN_PLACE_REVISITS_HPP
#define GENMC_IN_PLACE_REVISITS_HPP

/*
 * The reads a write revisits in place: the read takes the write as its rf
 * in the current execution instead of in a new one, as nothing po-after
 * the read depends on it (the thread waits on it). Barrier-aware model
 * checking (BAM) revisits a round's waits from its last increment, and
 * in-place revisiting (IPR) revisits the reads blocked on an annotation.
 * Each a pure function of the graph; the revisiting is the driver's job.
 */

#include "genmc/Support/Event.hpp"

#include <vector>

class BIncFaiWriteLabel;
class ReadLabel;

namespace in_place {

/** BAM: the waits of the barrier round that SLAB, the round's last
 * increment, completes (the waits after the round's earlier increments) */
auto barrierWaits(const BIncFaiWriteLabel &sLab) -> std::vector<Event>;

/** IPR: whether RLAB, an annotated read that is not a CAS, blocks on the
 * value it reads */
auto blocksOnAnnotation(const ReadLabel &rLab) -> bool;

} // namespace in_place

#endif /* GENMC_IN_PLACE_REVISITS_HPP */
