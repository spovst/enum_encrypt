#include <stdlib.h>
#include <string.h>

#include "encryption.h"

#include "util.h"

static void
ee_sdata_common_crypt_s(ee_sdata_t *sdata, ee_key_t *key);
static ee_byte_t
ee_key_extract_next_byte_s(ee_key_t *key, ee_size_t bits_number);
static ee_size_t
ee_key_bit_get_s(ee_key_t *key);
static void
ee_key_bit_inc_s(ee_key_t *key);

ee_int_t
ee_key_init(ee_key_t *key, const ee_char_t *key_data)
{
    ee_char_t *buffer = NULL;
    ee_size_t buffer_len = 0;
    ee_int_t status = EE_SUCCESS;

    buffer_len = strlen(key_data);
    buffer = calloc(buffer_len + 1, sizeof(*buffer));
    if (NULL == buffer) {
        status = EE_ALLOC_FAILURE;
        goto calloc_error;
    }

    strcpy(buffer, key_data);

    key->key = buffer;
    key->length = buffer_len;
    key->bit_info.current_bit = EE_BITS_IN_BYTE - 1;
    key->bit_info.current_byte = 0;

calloc_error:
    return status;
}

void
ee_key_deinit(ee_key_t *key)
{
    free(key->key);
    ee_memset(key, 0, sizeof(*key));
}

void
ee_sdata_encrypt(ee_sdata_t *sdata, ee_key_t *key)
{
    ee_sdata_common_crypt_s(sdata, key);
}

void
ee_sdata_decrypt(ee_sdata_t *sdata, ee_key_t *key)
{
    ee_sdata_common_crypt_s(sdata, key);
}

static void
ee_sdata_common_crypt_s(ee_sdata_t *sdata, ee_key_t *key)
{
    ee_size_t bytes_number = EE_EVAL_BYTES_NUMBER(sdata->bits_number);

    for (ee_size_t i = 0; i < bytes_number; ++i) {
        ee_size_t bits_number = EE_BITS_IN_BYTE;
        if (i == bytes_number - 1) {
            bits_number = sdata->bits_number - EE_BITS_IN_BYTE * i;
        }

        ee_byte_t key_byte = ee_key_extract_next_byte_s(key, bits_number);
        sdata->bytes[i] ^= key_byte;
    }
}

static ee_byte_t
ee_key_extract_next_byte_s(ee_key_t *key, ee_size_t bits_number) {
    ee_byte_t byte = 0;

    for (ee_size_t i = bits_number; i > 0; --i) {
        byte |= ee_key_bit_get_s(key) << (i - 1);
        ee_key_bit_inc_s(key);
    }

    return byte;
}

static ee_size_t
ee_key_bit_get_s(ee_key_t *key)
{
    return ee_bit_get(key->key[key->bit_info.current_byte],
            key->bit_info.current_bit);
}

static void
ee_key_bit_inc_s(ee_key_t *key)
{
    ee_bit_info_ms_inc_cyc(&(key->bit_info), key->length);
}
