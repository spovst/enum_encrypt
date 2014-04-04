#ifndef NUMERATION_H
#define	NUMERATION_H

#include <gmp.h>

#include "common.h"
#include "block.h"
#include "statistics.h"

typedef struct ee_number_s {
    mpz_t eta;
    mpz_t delta;
} ee_number_t;

typedef struct ee_subnumber_s {
    ee_int_t subset;
    mpz_t subnum;
    ee_size_t subnum_bit_length;
} ee_subnumber_t;

void
ee_number_init(ee_number_t *number);
void
ee_number_deinit(ee_number_t *number);

void
ee_subnumber_init(ee_subnumber_t *subnumber);
void
ee_subnumber_deinit(ee_subnumber_t *subnumber);

ee_int_t
ee_number_eval(ee_number_t *number, ee_block_t *block,
        ee_statistics_t *statistics);
void
ee_subnumber_eval(ee_subnumber_t *subnumber, ee_number_t *number);

ee_int_t
ee_eval_rho(mpz_t out_rho, ee_block_t *block, ee_statistics_t *statistics);
ee_int_t
ee_eval_delta(mpz_t out_delta, mpz_t rho, ee_block_t *block,
        ee_statistics_t *statistics);
void
ee_eval_subnum_bit_length(ee_size_t *subnum_bit_length, mpz_t delta,
        ee_int_t subset);
void
ee_number_restore(ee_number_t *number, mpz_t delta, ee_subnumber_t *subnumber);
ee_int_t
ee_block_restore(ee_block_t *block, ee_statistics_t *statistics, mpz_t rho,
        ee_number_t *number);

#endif /* NUMERATION_H */
