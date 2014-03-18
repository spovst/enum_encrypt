#include <stdlib.h>
#include <string.h>

#include "statistics.h"

#include "common.h"
#include "util.h"

void
ee_statistics_gather(ee_statistics_t *statistics, ee_block_t *block)
{
    statistics->padding = block->size - block->length;
    ee_memset(statistics->stats, 0, sizeof(statistics->stats));
    for (ee_size_t i = 0; i < block->size; ++i) {
        statistics->stats[(ee_size_t)block->chars[i]] += 1;
    }
}

void
ee_statistics_eval_padding(ee_statistics_t *statistics, ee_size_t sigma)
{
    statistics->padding = 1 << sigma;
    for (ee_size_t i = 0; i < EE_ALPHABET_SIZE; ++i) {
        statistics->padding -= statistics->stats[i];
    }
}
