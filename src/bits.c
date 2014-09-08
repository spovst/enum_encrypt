#include "bits.h"

ee_size_t
ee_bit_get(ee_int_t value, ee_size_t bit_idx)
{
    return (value & (1 << bit_idx)) >> bit_idx;
}

ee_int_t
ee_bit_set(ee_int_t value, ee_size_t bit_idx, ee_size_t bit)
{
    if (bit == 0) {
        value &= ~(1 << bit_idx);
    } else {
        value |= 1 << bit_idx;
    }

    return value;
}

void
ee_bit_info_ls_inc(ee_bit_info_t *bit_info)
{
    bit_info->current_bit += 1;
    if (bit_info->current_bit >= EE_BITS_IN_BYTE) {
        bit_info->current_bit = 0;
        bit_info->current_byte += 1;
    }
}

void
ee_bit_info_ls_inc_cyc(ee_bit_info_t *bit_info, ee_size_t bytes_number)
{
    ee_bit_info_ls_inc(bit_info);
    if (bit_info->current_byte >= bytes_number) {
        bit_info->current_byte = 0;
    }
}

void
ee_bit_info_ms_inc(ee_bit_info_t *bit_info)
{
    bit_info->current_bit -= 1;
    if (bit_info->current_bit == (ee_size_t)(-1)) {
        bit_info->current_bit = EE_BITS_IN_BYTE - 1;
        bit_info->current_byte += 1;
    }
}

void
ee_bit_info_ms_inc_cyc(ee_bit_info_t *bit_info, ee_size_t bytes_number)
{
    ee_bit_info_ms_inc(bit_info);
    if (bit_info->current_byte >= bytes_number) {
        bit_info->current_byte = 0;
    }
}
