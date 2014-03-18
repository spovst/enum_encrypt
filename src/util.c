#include "util.h"

#include "common.h"

void
ee_memset(void *mem, ee_int_t val, ee_size_t count)
{
    ee_byte_t *bmem = (ee_byte_t *)mem;
    
    for (ee_size_t i = 0; i < count; ++i) {
        bmem[i] = val;
    }
}
