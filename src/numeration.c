#include <stdlib.h>
#include <string.h>
#include <gmp.h>
#include <ctype.h>

#include "numeration.h"

#include "common.h"
#include "block.h"
#include "statistics.h"
#include "delta.h"

typedef struct ee_z_item_s {
    mpz_t item;
    ee_bool_t init;
} ee_z_item_t;

typedef struct ee_rt_item_s {
    mpz_t rho;
    mpz_t theta;
} ee_rt_item_t;

static void
ee_eval_rt0_s(mpz_t *rho, mpz_t *theta, ee_block_t *block,
        ee_statistics_t *statistics);
static void
ee_eval_rt_s(mpz_t *rho, mpz_t *theta, ee_block_t *block, ee_deltas_t *deltas);

static void
ee_eval_z_s(ee_z_item_t **z, ee_int_t *indexes, ee_size_t sym_idx,
        ee_block_t *block, ee_deltas_t *deltas, ee_rt_item_t **rt);
static void
ee_block_restore_symbol_s(ee_block_t *block, ee_size_t sym_idx,
        ee_rt_item_t **rt, ee_int_t *thetas, ee_z_item_t **z,
        ee_deltas_t *deltas);

void
ee_number_init(ee_number_t *number)
{
    mpz_init(number->eta);
    mpz_init(number->delta);
}

void
ee_number_deinit(ee_number_t *number)
{
    mpz_clear(number->eta);
    mpz_clear(number->delta);
}

void
ee_subnumber_init(ee_subnumber_t *subnumber)
{
    subnumber->subset = 0;
    subnumber->subnum_bit_length = 0;
    mpz_init(subnumber->subnum);
}

void
ee_subnumber_deinit(ee_subnumber_t *subnumber)
{
    subnumber->subset = 0;
    subnumber->subnum_bit_length = 0;
    mpz_clear(subnumber->subnum);
}

ee_int_t
ee_number_eval(ee_number_t *number, ee_block_t *block,
        ee_statistics_t *statistics, ee_deltas_t *deltas)
{
    ee_int_t status = EE_SUCCESS;
    mpz_t *rho = NULL;
    mpz_t *theta = NULL;
    
    rho = calloc(block->size, sizeof(*rho));
    if (NULL == rho) {
        status = EE_ALLOC_FAILURE;
        goto rho_calloc_error;
    }
    
    theta = calloc(block->size, sizeof(*theta));
    if (NULL == theta) {
        status = EE_ALLOC_FAILURE;
        goto theta_calloc_error;
    }
    
    for (ee_size_t i = 0; i < block->size; ++i) {
        mpz_init(rho[i]);
        mpz_init(theta[i]);
    }
    
    ee_eval_rt0_s(rho, theta, block, statistics);
    ee_eval_rt_s(rho, theta, block, deltas);
    
    mpz_cdiv_q(number->eta, theta[0], rho[0]);
    mpz_cdiv_q(number->delta, deltas->deltas[deltas->sigma][0], rho[0]);
    
    for (ee_size_t i = 0; i < block->size; ++i) {
        mpz_clear(rho[i]);
        mpz_clear(theta[i]);
    }
    
    free(theta);
theta_calloc_error:
    free(rho);
rho_calloc_error:
    return status;
}

void
ee_subnumber_eval(ee_subnumber_t *subnumber, ee_number_t *number)
{
    ee_size_t bit_idx;
    mpz_t diff;
    
    mpz_init(diff);
    
    mpz_set(subnumber->subnum, number->eta);
    subnumber->subset = 0;
    for (bit_idx = mpz_sizeinbase(number->delta, 2); 0 != bit_idx; --bit_idx) {
        if (0 != mpz_tstbit(number->delta, bit_idx)) {
            mpz_set_ui(diff, 1);
            mpz_mul_2exp(diff, diff, bit_idx);
            if (mpz_cmp(subnumber->subnum, diff) >= 0) {
                mpz_sub(subnumber->subnum, subnumber->subnum, diff);
                subnumber->subset += 1;
            } else {
                break;
            }
        }
    }
    
    subnumber->subnum_bit_length = bit_idx;
    
    mpz_clear(diff);
}

ee_int_t
ee_eval_rho(mpz_t out_rho, ee_block_t *block, ee_statistics_t *statistics)
{
    ee_int_t status = EE_SUCCESS;
    mpz_t *rho = NULL;
    
    rho = calloc(block->size, sizeof(*rho));
    if (NULL == rho) {
        status = EE_ALLOC_FAILURE;
        goto rho_calloc_error;
    }
    
    for (ee_size_t i = 0; i < block->size; ++i) {
        mpz_init(rho[i]);
    }
    
    ee_eval_rt0_s(rho, NULL, block, statistics);
    ee_eval_rt_s(rho, NULL, block, NULL);
    
    mpz_set(out_rho, rho[0]);
    
    for (ee_size_t i = 0; i < block->size; ++i) {
        mpz_clear(rho[i]);
    }
    
    free(rho);
rho_calloc_error:
    return status;
}

void
ee_eval_subnum_bit_length(ee_size_t *subnum_bit_length, mpz_t delta,
        ee_int_t subset)
{
    *subnum_bit_length = mpz_sizeinbase(delta, 2);
    for (ee_size_t i = 0; i <= subset; ++i) {
        do {
            *subnum_bit_length -= 1;
        } while (0 == mpz_tstbit(delta, *subnum_bit_length));
    }
}

void
ee_number_restore(ee_number_t *number, mpz_t delta, ee_subnumber_t *subnumber)
{
    ee_size_t bit_idx;
    mpz_t diff;
    
    mpz_init(diff);
    
    bit_idx = mpz_sizeinbase(delta, 2);
    while (0 == mpz_tstbit(delta, bit_idx)) {
        bit_idx -= 1;
    }
    
    mpz_set(number->eta, subnumber->subnum);
    mpz_set(number->delta, delta);
    for (ee_size_t i = subnumber->subset; i > 0; --i) {
        while (0 == mpz_tstbit(delta, bit_idx)) {
            bit_idx -= 1;
        }
        
        mpz_set_ui(diff, 1);
        mpz_mul_2exp(diff, diff, bit_idx);
        mpz_add(number->eta, number->eta, diff);
        bit_idx -= 1;
    }
    
    mpz_clear(diff);
}

ee_int_t
ee_block_restore(ee_block_t *block, ee_statistics_t *statistics, mpz_t rho,
        ee_deltas_t *deltas, ee_number_t *number)
{   
    ee_int_t status = EE_SUCCESS;
    ee_int_t *thetas = NULL;
    ee_int_t *indexes = NULL;
    ee_z_item_t **z = NULL;
    ee_rt_item_t **rt = NULL;
    ee_size_t zrows = block->sigma + 1;
    
    thetas = calloc(EE_ALPHABET_SIZE + 1, sizeof(*thetas));
    if (NULL == thetas) {
        status = EE_ALLOC_FAILURE;
        goto thetas_calloc_error;
    }
    
    indexes = calloc(block->sigma + 1, sizeof(*indexes));
    if (NULL == indexes) {
        status = EE_ALLOC_FAILURE;
        goto indexes_calloc_error;
    }
    
    z = calloc(zrows, sizeof(*z));
    if (NULL == z) {
        status = EE_ALLOC_FAILURE;
        goto z_calloc_error;
    }
    for (ee_size_t i = 0; i < zrows; ++i) {
        z[i] = calloc(block->size >> i, sizeof(**z));
        if (NULL == z[i]) {
            status = EE_ALLOC_FAILURE;
            goto z_i_calloc_error;
        }
    }
    
    rt = calloc(block->sigma, sizeof(*z));
    if (NULL == z) {
        status = EE_ALLOC_FAILURE;
        goto rt_calloc_error;
    }
    for (ee_size_t i = 0; i < block->sigma; ++i) {
        rt[i] = calloc((block->size >> i) - 1, sizeof(**rt));
        if (NULL == rt[i]) {
            status = EE_ALLOC_FAILURE;
            goto rt_i_calloc_error;
        }
    }
    
    for (ee_size_t i = 0; i < zrows; ++i) {
        ee_size_t cols = block->size >> i;
        for (ee_size_t j = 0; j < cols; ++j) {
            mpz_init(z[i][j].item);
        }
    }
    
    for (ee_size_t i = 0; i < block->sigma; ++i) {
        ee_size_t cols = (block->size >> i) - 1;
        for (ee_size_t j = 0; j < cols; ++j) {
            mpz_init(rt[i][j].rho);
            mpz_init(rt[i][j].theta);
        }
    }
    
    thetas[0] = 0;
    for (ee_size_t i = 0; i < EE_ALPHABET_SIZE; ++i) {
        thetas[i + 1] = thetas[i] + statistics->stats[i];
    }
    
    mpz_mul(z[block->sigma][0].item, rho, number->eta);
    z[block->sigma][0].init = EE_TRUE;
    for (ee_size_t i = block->sigma; i > 0; --i) {
        mpz_fdiv_q(z[i - 1][0].item, z[i][0].item, deltas->deltas[i - 1][1]);
        z[i - 1][0].init = EE_TRUE;
    }
    
    for (ee_size_t i = 0; i < block->size; ++i) {
        if (EE_FALSE == z[0][i].init) {
            ee_eval_z_s(z, indexes, i, block, deltas, rt);
        }
        
        ee_block_restore_symbol_s(block, i, rt, thetas, z, deltas);
    }
    
    block->length -= statistics->padding;
    
    for (ee_size_t i = 0; i < block->sigma; ++i) {
        ee_size_t cols = (block->size >> i) - 1;
        for (ee_size_t j = 0; j < cols; ++j) {
            mpz_clear(rt[i][j].rho);
            mpz_clear(rt[i][j].theta);
        }
    }
    
    for (ee_size_t i = 0; i < zrows; ++i) {
        ee_size_t cols = block->size >> i;
        for (ee_size_t j = 0; j < cols; ++j) {
            mpz_clear(z[i][j].item);
        }
    }
    
    for (ee_size_t i = 0; i < block->sigma; ++i) {
        free(rt[i]);
    }
rt_i_calloc_error:
    free(rt);
rt_calloc_error:
    for (ee_size_t i = 0; i < zrows; ++i) {
        free(z[i]);
    }
z_i_calloc_error:
    free(z);
z_calloc_error:
    free(indexes);
indexes_calloc_error:
    free(thetas);
thetas_calloc_error:
    return status;
}

static void
ee_eval_rt0_s(mpz_t *rho, mpz_t *theta, ee_block_t *block,
        ee_statistics_t *statistics)
{
    ee_int_t iter_stats[EE_ALPHABET_SIZE];
    ee_int_t tmp_theta;
    
    if (NULL != rho) {
        memcpy(iter_stats, statistics->stats, sizeof(iter_stats));
        mpz_set_ui(rho[0], iter_stats[(ee_size_t)block->chars[0]]);
    }
    
    if (NULL != theta) {
        tmp_theta = 0;
        for (ee_size_t i = 1; i < block->size; ++i) {
            if (block->chars[i] < block->chars[0]) {
                tmp_theta += 1;
            }
        }
        
        mpz_set_ui(theta[0], tmp_theta);
    }
    
    for (ee_size_t i = 1; i < block->size; ++i) {
        if (NULL != rho) {
            iter_stats[(ee_size_t)block->chars[i - 1]] -= 1;
            mpz_set_ui(rho[i], iter_stats[(ee_size_t)block->chars[i]]);
        }

        if (NULL != theta) {
            tmp_theta = 0;
            for (ee_size_t j = i + 1; j < block->size; ++j) {
                if (block->chars[j] < block->chars[i]) {
                    tmp_theta += 1;
                }
            }

            mpz_set_ui(theta[i], tmp_theta);
        }
    }
}

static void
ee_eval_rt_s(mpz_t *rho, mpz_t *theta, ee_block_t *block, ee_deltas_t *deltas)
{
    mpz_t tmp1, tmp2;
    
    if (NULL != theta && NULL != deltas) {
        mpz_init(tmp1);
        mpz_init(tmp2);
    }
    
    for (ee_size_t i = 1; i <= block->sigma; ++i) {
        ee_size_t jmax = 1 << (block->sigma - i);
        for (ee_size_t j = 1; j <= jmax; ++j) {
            if (NULL != theta && NULL != rho && NULL != deltas) {
                mpz_mul(tmp1, theta[2 * j - 2], deltas->deltas[i - 1][2 * j - 1]);
                mpz_mul(tmp2, rho[2 * j - 2], theta[2 * j - 1]);
                mpz_add(theta[j - 1], tmp1, tmp2);
            }
            
            if (NULL != rho) {
                mpz_mul(rho[j - 1], rho[2 * j - 2], rho[2 * j - 1]);
            }
        }
    }
    
    if (NULL != theta && NULL != deltas) {
        mpz_clear(tmp2);
        mpz_clear(tmp1);
    }
}

static void
ee_eval_z_s(ee_z_item_t **z, ee_int_t *indexes, ee_size_t sym_idx,
        ee_block_t *block, ee_deltas_t *deltas, ee_rt_item_t **rt)
{
    ee_size_t cur_z;
    mpz_t tmp1, tmp2;
    
    mpz_init(tmp1);
    mpz_init(tmp2);
    
    indexes[0] = sym_idx;
    for (cur_z = 1; cur_z < block->sigma + 1; ++cur_z) {
        indexes[cur_z] = indexes[cur_z - 1] / 2;
        if (EE_TRUE == z[cur_z][indexes[cur_z]].init) {
            break;
        }
    }

    do {
        cur_z -= 1;
        if ((indexes[cur_z] & 0x01) == 1) {
            mpz_mul(tmp1, rt[cur_z][indexes[cur_z] - 1].theta,
                    deltas->deltas[cur_z][indexes[cur_z]]);
            mpz_sub(tmp2, z[cur_z + 1][indexes[cur_z + 1]].item, tmp1);
            mpz_fdiv_q(z[cur_z][indexes[cur_z]].item, tmp2,
                       rt[cur_z][indexes[cur_z] - 1].rho);
        } else {
            mpz_fdiv_q(z[cur_z][indexes[cur_z]].item,
                       z[cur_z + 1][indexes[cur_z + 1]].item,
                       deltas->deltas[cur_z][indexes[cur_z] + 1]);
        }

        z[cur_z][indexes[cur_z]].init = EE_TRUE;
    } while (cur_z != 0);
    
    mpz_clear(tmp2);
    mpz_clear(tmp1);
}

static void
ee_block_restore_symbol_s(ee_block_t *block, ee_size_t sym_idx,
        ee_rt_item_t **rt, ee_int_t *thetas, ee_z_item_t **z,
        ee_deltas_t *deltas)
{
    mpz_t tmp1, tmp2;
    
    mpz_init(tmp1);
    mpz_init(tmp2);
    
    ee_size_t ch;
    for (ch = 0; ch < EE_ALPHABET_SIZE; ++ch) {
        if (       mpz_cmp_ui(z[0][sym_idx].item, thetas[ch]) >= 0
                && mpz_cmp_ui(z[0][sym_idx].item, thetas[ch + 1]) < 0) {
            block->chars[sym_idx] = (ee_char_t)ch;
            if (sym_idx == block->size - 1) {
                break;
            }

            mpz_set_ui(rt[0][sym_idx].rho, thetas[ch + 1] - thetas[ch]);
            mpz_set_ui(rt[0][sym_idx].theta, thetas[ch]);
            for (   ee_size_t k = sym_idx, l = 0;
                    ((k & 0x01) == 1) && (l < block->sigma - 1);
                    k /= 2, l++) {
                mpz_mul(rt[l + 1][k / 2].rho, rt[l][k - 1].rho, rt[l][k].rho);
                mpz_mul(tmp1, rt[l][k - 1].theta, deltas->deltas[l][k]);
                mpz_mul(tmp2, rt[l][k - 1].rho, rt[l][k].theta);
                mpz_add(rt[l + 1][k / 2].theta, tmp1, tmp2);
            }

            break;
        }
    }
    
    mpz_clear(tmp2);
    mpz_clear(tmp1);
    
    for (ee_size_t j = ch + 1; j <= EE_ALPHABET_SIZE; ++j) {
        thetas[j] -= 1;
    }
}
