/* A seqlock reader that reads the data relaxed: the data can be newer than the
 * sequence number it read twice, under RC11 and IMM. Its reads of the sequence
 * number are revisited in place (tests/correct/saver/spin-seqlock-reader) */
#define BUG_RELAXED_DATA
#include "../../../../correct/saver/spin-seqlock-reader/variants/spin-seqlock-reader0.c"
