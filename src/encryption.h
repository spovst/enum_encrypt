#ifndef ENCRYPTION_H
#define	ENCRYPTION_H

#include "common.h"
#include "bits.h"
#include "serializer.h"

typedef struct ee_key_s {
    ee_char_t *key;
    ee_size_t length;
    ee_bit_info_t bit_info;
} ee_key_t;

ee_int_t
ee_key_init(ee_key_t *key, const ee_char_t *key_data);
void
ee_key_deinit(ee_key_t *key);

void
ee_sdata_encrypt(ee_sdata_t *sdata, ee_key_t *key);
void
ee_sdata_decrypt(ee_sdata_t *sdata, ee_key_t *key);

#endif /* ENCRYPTION_H */
