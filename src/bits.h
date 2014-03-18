#ifndef BITS_H
#define	BITS_H

#include "common.h"

#define EE_BIT_INFO_DEFAULT \
        { \
            .current_byte = 0, \
            .current_bit = EE_BITS_IN_BYTE - 1 \
        }

typedef struct ee_bit_info_s {
    ee_size_t current_byte;
    ee_size_t current_bit;
} ee_bit_info_t;

ee_size_t
ee_bit_get(ee_int_t value, ee_size_t bit_idx);
ee_int_t
ee_bit_set(ee_int_t value, ee_size_t bit_idx, ee_size_t bit);
void
ee_bit_info_ls_inc(ee_bit_info_t *bit_info);
void
ee_bit_info_ls_inc_cyc(ee_bit_info_t *bit_info, ee_size_t bytes_number);
void
ee_bit_info_ms_inc(ee_bit_info_t *bit_info);
void
ee_bit_info_ms_inc_cyc(ee_bit_info_t *bit_info, ee_size_t bytes_number);

#endif /* BITS_H */

