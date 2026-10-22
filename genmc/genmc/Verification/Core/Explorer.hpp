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


#ifndef GENMC_EXPLORER_HPP
#define GENMC_EXPLORER_HPP

class GenMCDriver;
class EventProducer;

/**
 * Explores PRODUCER's program with DRIVER until the driver is done: one
 * run of the program per execution, each from its start or from a resume
 * point the driver saved. The result is in DRIVER.getResult().
 */
void explore(GenMCDriver &driver, EventProducer &producer);

#endif /* GENMC_EXPLORER_HPP */
