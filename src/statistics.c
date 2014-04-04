#include <stdlib.h>
#include <string.h>

#include "statistics.h"

#include "common.h"
#include "util.h"

void
ee_statistics_gather(ee_statistics_t *statistics, ee_block_t *block)
{
    ee_memset(statistics->stats, 0, sizeof(statistics->stats));
    for (ee_size_t i = 0; i < block->length; ++i) {
        statistics->stats[(ee_size_t)block->chars[i]] += 1;
    }
}
