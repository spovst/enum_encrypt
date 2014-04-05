#ifndef CRYPT_H
#define	CRYPT_H

#include "common.h"
#include "io.h"

ee_int_t
ee_encrypt(ee_file_t *outfile, ee_file_t *infile, const ee_char_t *key_data,
        ee_size_t sigma, ee_size_t mu);
ee_int_t
ee_decrypt(ee_file_t *outfile, ee_file_t *infile, const ee_char_t *key_data,
        ee_size_t sigma, ee_size_t mu);

#endif /* CRYPT_H */
