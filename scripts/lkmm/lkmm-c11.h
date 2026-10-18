/*
 * GenMC's <lkmm.h> (the kernel primitives mapped onto C11 atomics) for the
 * C litmus tests that scripts/lkmm/litmus2c.py translates. herd7's
 * registers are untyped, so an address may be held in an integer register
 * and a pointer argument may be such an integer; and herd7's atomic_t is a
 * word, not a struct.
 */
#ifndef LKMM_C11_H
#define LKMM_C11_H

#include <stdint.h>

#define __lkmm_ptr(X) _Generic((X), intptr_t: (intptr_t *) (uintptr_t) (X), default: (X))
#define LKMM_WORD_ATOMIC
#include <lkmm.h>

#endif /* LKMM_C11_H */
