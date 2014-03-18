#ifndef SERIALIZER_H
#define	SERIALIZER_H

#include <gmp.h>

#include "common.h"
#include "statistics.h"
#include "numeration.h"

typedef struct ee_sdata_s {
    ee_byte_t *bytes;
    ee_size_t bits_number;
} ee_sdata_t;

void
ee_sdata_clear(ee_sdata_t *bits);

ee_int_t
ee_statistics_serialize(ee_sdata_t *data, ee_statistics_t *statistics,
        ee_size_t sigma);
void
ee_statistics_deserialize(ee_statistics_t *statistics, ee_sdata_t *data,
        ee_size_t sigma);

ee_int_t
ee_mpz_serialize(ee_sdata_t *data, mpz_t mpz, ee_size_t bits_number);
void
ee_mpz_deserialize(mpz_t mpz, ee_size_t bits_number, ee_sdata_t *data);

ee_int_t
ee_subset_serialize(ee_sdata_t *data, ee_int_t subset, ee_size_t sigma);
void
ee_subset_deserialize(ee_int_t *subset, ee_size_t sigma, ee_sdata_t *data);

#endif /* SERIALIZER_H */
