#ifndef CRYPT_H
#define	CRYPT_H

#include "common.h"
#include "io.h"

ee_int_t
ee_encypt(ee_file_t *outfile, ee_file_t *infile, const ee_char_t *key_data,
        ee_size_t sigma);
ee_int_t
ee_decypt(ee_file_t *outfile, ee_file_t *infile, const ee_char_t *key_data,
        ee_size_t sigma);

#endif /* CRYPT_H */
