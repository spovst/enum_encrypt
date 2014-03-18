#ifndef DELTA_H
#define	DELTA_H

#include <gmp.h>

#include "common.h"

typedef struct ee_deltas_s {
    mpz_t **deltas;
    ee_size_t sigma;
} ee_deltas_t;

ee_int_t
ee_deltas_init(ee_deltas_t *deltas, ee_size_t sigma);
void
ee_deltas_deinit(ee_deltas_t *deltas);

ee_int_t
ee_deltas_get(ee_deltas_t *deltas);

#endif /* DELTA_H */
