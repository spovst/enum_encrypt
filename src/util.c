#include <stdio.h>

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

ee_bool_t
ee_is_file_exists(const ee_char_t *name)
{
    FILE *file = NULL;

    file = fopen(name, "r");
    if (NULL == file) {
        return EE_FALSE;
    }

    fclose(file);

    return EE_TRUE;
}
