#include <stdlib.h>
#include <string.h>
#include <gmp.h>

#include "serializer.h"

#include "bits.h"
#include "util.h"

void
ee_sdata_clear(ee_sdata_t *data)
{
    if (NULL != data->bytes) {
        free(data->bytes);
    }

    ee_memset(data, 0, sizeof(*data));
}

ee_int_t
ee_statistics_serialize(ee_sdata_t *data, ee_statistics_t *statistics,
        ee_size_t sigma)
{
    ee_int_t status = EE_SUCCESS;
    ee_size_t item_size = sigma + 1;
    ee_size_t bytes_number;
    ee_bit_info_t bit_info = EE_BIT_INFO_DEFAULT;

    data->bits_number = item_size * EE_ALPHABET_SIZE;
    bytes_number = EE_EVAL_BYTES_NUMBER(data->bits_number);
    data->bytes = calloc(bytes_number, sizeof(ee_byte_t));
    if (NULL == data->bytes) {
        status = EE_ALLOC_FAILURE;
        goto calloc_error;
    }

    for (ee_size_t i = 0; i < EE_ALPHABET_SIZE; ++i) {
        for (ee_size_t j = item_size; j > 0; --j) {
            ee_size_t bit = ee_bit_get(statistics->stats[i], j - 1);
            ee_int_t value = data->bytes[bit_info.current_byte];
            data->bytes[bit_info.current_byte] = ee_bit_set(value,
                    bit_info.current_bit, bit);
            ee_bit_info_ms_inc(&bit_info);
        }
    }

calloc_error:
    return status;
}

void
ee_statistics_deserialize(ee_statistics_t *statistics, ee_sdata_t *data,
        ee_size_t sigma)
{
    ee_size_t item_size = sigma + 1;
    ee_bit_info_t bit_info = EE_BIT_INFO_DEFAULT;

    for (ee_size_t i = 0; i < EE_ALPHABET_SIZE; ++i) {
        statistics->stats[i] = 0;
        for (ee_size_t j = item_size; j > 0; --j) {
            ee_size_t bit = ee_bit_get(data->bytes[bit_info.current_byte],
                    bit_info.current_bit);
            statistics->stats[i] = ee_bit_set(statistics->stats[i], j - 1, bit);
            ee_bit_info_ms_inc(&bit_info);
        }
    }
}

ee_int_t
ee_mpz_serialize(ee_sdata_t *data, mpz_t mpz, ee_size_t bits_number)
{
    ee_int_t status = EE_SUCCESS;
    ee_size_t bytes_number = EE_EVAL_BYTES_NUMBER(bits_number);

    data->bytes = calloc(bytes_number, sizeof(ee_byte_t));
    data->bits_number = bits_number;
    if (NULL == data->bytes) {
        status = EE_ALLOC_FAILURE;
        goto calloc_error;
    }

    mpz_export(data->bytes, NULL, -1, sizeof(ee_byte_t), -1, 0, mpz);

calloc_error:
    return status;
}

void
ee_mpz_deserialize(mpz_t mpz, ee_size_t bits_number, ee_sdata_t *data)
{
    ee_size_t bytes_number = EE_EVAL_BYTES_NUMBER(bits_number);
    mpz_import(mpz, bytes_number, -1, sizeof(ee_byte_t), -1, 0, data->bytes);
}

ee_int_t
ee_subset_serialize(ee_sdata_t *data, ee_int_t subset, ee_size_t sigma)
{
    ee_int_t status = EE_SUCCESS;
    ee_size_t bytes_number;
    ee_bit_info_t bit_info = { 0, 0 };

    data->bits_number = sigma + 4;
    bytes_number = EE_EVAL_BYTES_NUMBER(data->bits_number);
    data->bytes = calloc(bytes_number, sizeof(ee_byte_t));
    if (NULL == data->bytes) {
        status = EE_ALLOC_FAILURE;
        goto calloc_error;
    }

    for (ee_size_t i = data->bits_number; i > 0; --i) {
        ee_size_t bit = ee_bit_get(subset, i - 1);
        ee_byte_t value = data->bytes[bit_info.current_byte];
        data->bytes[bit_info.current_byte] = ee_bit_set(value,
                bit_info.current_bit, bit);
        ee_bit_info_ls_inc(&bit_info);
    }

calloc_error:
    return status;
}

void
ee_subset_deserialize(ee_int_t *subset, ee_sdata_t *data)
{
    ee_bit_info_t bit_info = { 0, 0 };

    *subset = 0;
    for (ee_size_t i = data->bits_number; i > 0; --i) {
        ee_size_t bit = ee_bit_get(data->bytes[bit_info.current_byte],
                bit_info.current_bit);
        *subset = ee_bit_set(*subset, i - 1, bit);
        ee_bit_info_ls_inc(&bit_info);
    }
}

ee_int_t
ee_source_info_serialize(ee_sdata_t *data, ee_source_t *source, ee_size_t mu)
{
    ee_int_t status = EE_SUCCESS;
    ee_size_t bytes_number;
    ee_bit_info_t bit_info = EE_BIT_INFO_DEFAULT;

    data->bits_number = (mu + 1 + 4) * EE_BITS_IN_BYTE;
    bytes_number = EE_EVAL_BYTES_NUMBER(data->bits_number);
    data->bytes = calloc(bytes_number, sizeof(ee_byte_t));
    if (NULL == data->bytes) {
        status = EE_ALLOC_FAILURE;
        goto calloc_error;
    }

    memcpy(data->bytes, source->prefix, mu);
    data->bytes[mu] = source->chars[source->length - 1];
    bit_info.current_byte = mu + 1;
    for (ee_size_t i = 0; i < 4 * EE_BITS_IN_BYTE; ++i) {
        ee_size_t bit = ee_bit_get(source->length, i);
        ee_int_t value = data->bytes[bit_info.current_byte];
        value = ee_bit_set(value, bit_info.current_bit, bit);
        data->bytes[bit_info.current_byte] = value;
        ee_bit_info_ms_inc(&bit_info);
    }

calloc_error:
    return status;
}

void
ee_source_info_deserialize(ee_source_t *source, ee_char_t *last_char,
        ee_size_t *length, ee_sdata_t *data, ee_size_t mu)
{
    ee_bit_info_t bit_info = EE_BIT_INFO_DEFAULT;

    ee_memset(source->prefix, 0, mu);
    memcpy(source->prefix, data->bytes, mu);
    *last_char = data->bytes[mu];
    bit_info.current_byte = mu + 1;
    for (ee_size_t i = 0; i < 4 * EE_BITS_IN_BYTE; ++i) {
        ee_size_t bit = ee_bit_get(data->bytes[bit_info.current_byte],
                bit_info.current_bit);
        *length = ee_bit_set(*length, i, bit);
        ee_bit_info_ms_inc(&bit_info);
    }
}
