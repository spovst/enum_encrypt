#ifndef STATISTICS_H
#define	STATISTICS_H

#include "common.h"

typedef struct ee_statistics_s {
    ee_int_t stats[EE_ALPHABET_SIZE];
    ee_size_t padding;
} ee_statistics_t;

#include "block.h"

void
ee_statistics_gather(ee_statistics_t *statistics, ee_block_t *block);
void
ee_statistics_eval_padding(ee_statistics_t *statistics, ee_size_t sigma);

#endif /* STATISTICS_H */
