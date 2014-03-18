#ifndef BLOCK_H
#define	BLOCK_H

#include "common.h"

typedef struct ee_block_s {
    ee_size_t sigma;
    ee_size_t size;
    ee_size_t length;
    ee_char_t *chars;
} ee_block_t;

#include "statistics.h"

ee_int_t
ee_block_init(ee_block_t *block, ee_size_t sigma);
void
ee_block_deinit(ee_block_t *block);

void
ee_block_generate(ee_block_t *block, ee_statistics_t *statistics);

#endif /* BLOCK_H */
