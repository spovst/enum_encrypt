#include <stdlib.h>

#include "block.h"

#include "common.h"
#include "statistics.h"
#include "util.h"

ee_int_t
ee_block_init(ee_block_t *block, ee_size_t sigma)
{
    ee_int_t status = EE_SUCCESS;
    
    block->sigma = sigma;
    block->size = 1 << sigma;
    block->length = 0;
    block->chars = calloc(block->size, sizeof(*(block->chars)));
    if (NULL == block->chars) {
        status = EE_ALLOC_FAILURE;
    }
    
    return status;
}

void
ee_block_deinit(ee_block_t *block)
{
    free(block->chars);
    ee_memset(block, 0, sizeof(*block));
}

void
ee_block_generate(ee_block_t *block, ee_statistics_t *statistics)
{
    ee_char_t *ch = NULL;
    
    block->length = 0;
    ee_memset(block->chars, 0, block->size * sizeof(*(block->chars)));
    ch = block->chars;
    for (ee_size_t i = 0; i < EE_ALPHABET_SIZE; ++i) {
        for (ee_size_t j = 0; j < statistics->stats[i]; ++j) {
            *ch = (ee_char_t)i;
            block->length += 1;
            ch += 1;
        }
    }
}
