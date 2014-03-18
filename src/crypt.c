#include <stdio.h>

#include <gmp.h>

#include "crypt.h"

#include "common.h"
#include "io.h"
#include "delta.h"
#include "statistics.h"
#include "block.h"
#include "numeration.h"
#include "encryption.h"
#include "serializer.h"
#include "bits.h"

#define EE_GOTO_IF_NOT_SUCCESS(status, label) \
        if (EE_SUCCESS != (status)) { \
            goto label; \
        }

#define EE_BREAK_IF_NOT_SUCCESS(status) \
        if (EE_SUCCESS != (status)) { \
            break; \
        }

#define EE_BREAK_IF_FAILURE(status) \
        if (EE_FAILURE == (status)) { \
            break; \
        }

ee_int_t
ee_encypt(ee_file_t *outfile, ee_file_t *infile, const ee_char_t *key_data,
        ee_size_t sigma)
{
    ee_int_t status;
    ee_int_t block_status;
    
    ee_deltas_t deltas;
    ee_statistics_t statistics;
    
    ee_block_t block;
    ee_number_t number;
    ee_subnumber_t subnumber;
    ee_key_t key;
    
    ee_sdata_t statistics_data = { .bytes = NULL, .bits_number = 0 };
    ee_sdata_t subnum_data = { .bytes = NULL, .bits_number = 0 };
    ee_sdata_t subset_data = { .bytes = NULL, .bits_number = 0 };
    
    status = ee_deltas_init(&deltas, sigma);
    EE_GOTO_IF_NOT_SUCCESS(status, deltas_init_error);
    status = ee_deltas_get(&deltas);
    EE_GOTO_IF_NOT_SUCCESS(status, deltas_get_error);
    status = ee_block_init(&block, sigma);
    EE_GOTO_IF_NOT_SUCCESS(status, block_init_error);
    status = ee_key_init(&key, key_data);
    EE_GOTO_IF_NOT_SUCCESS(status, key_init_error);
    
    ee_number_init(&number);
    ee_subnumber_init(&subnumber);
    
    do {
        status = ee_file_read_block(&block, infile);
        EE_BREAK_IF_FAILURE(status);
        block_status = status;
        status = EE_SUCCESS;
        ee_statistics_gather(&statistics, &block);
        status = ee_number_eval(&number, &block, &statistics, &deltas);
        EE_BREAK_IF_NOT_SUCCESS(status);
        ee_subnumber_eval(&subnumber, &number);
        status = ee_mpz_serialize(&subnum_data, subnumber.subnum,
                subnumber.subnum_bit_length);
        EE_BREAK_IF_NOT_SUCCESS(status);
        ee_sdata_encrypt(&subnum_data, &key);
        status = ee_statistics_serialize(&statistics_data, &statistics, sigma);
        EE_BREAK_IF_NOT_SUCCESS(status);
        status = ee_subset_serialize(&subset_data, subnumber.subset, sigma);
        EE_BREAK_IF_NOT_SUCCESS(status);
        status = ee_file_write_sdata(outfile, &statistics_data);
        EE_BREAK_IF_NOT_SUCCESS(status);
        status = ee_file_write_sdata(outfile, &subset_data);
        EE_BREAK_IF_NOT_SUCCESS(status);
        status = ee_file_write_sdata(outfile, &subnum_data);
        EE_BREAK_IF_NOT_SUCCESS(status);
        ee_sdata_clear(&subset_data);
        ee_sdata_clear(&subnum_data);
        ee_sdata_clear(&statistics_data);
    } while (EE_FINAL_BLOCK != block_status);
    
    ee_sdata_clear(&subset_data);
    ee_sdata_clear(&subnum_data);
    ee_sdata_clear(&statistics_data);
    
    ee_subnumber_deinit(&subnumber);
    ee_number_deinit(&number);
    
    ee_key_deinit(&key);
key_init_error:
    ee_block_deinit(&block);
block_init_error:
deltas_get_error:    
    ee_deltas_deinit(&deltas);
deltas_init_error:
    return status;
}

ee_int_t
ee_decypt(ee_file_t *outfile, ee_file_t *infile, const ee_char_t *key_data,
        ee_size_t sigma)
{
    ee_int_t status;
    
    ee_deltas_t deltas;
    ee_statistics_t statistics;
    
    ee_block_t block;
    ee_number_t number;
    ee_subnumber_t subnumber;
    ee_key_t key;
    
    ee_sdata_t statistics_data = { .bytes = NULL, .bits_number = 0 };
    ee_sdata_t subnum_data = { .bytes = NULL, .bits_number = 0 };
    ee_sdata_t subset_data = { .bytes = NULL, .bits_number = 0 };
    
    mpz_t rho;
    mpz_t delta;
    
    status = ee_deltas_init(&deltas, sigma);
    EE_GOTO_IF_NOT_SUCCESS(status, deltas_init_error);
    status = ee_deltas_get(&deltas);
    EE_GOTO_IF_NOT_SUCCESS(status, deltas_get_error);
    status = ee_block_init(&block, sigma);
    EE_GOTO_IF_NOT_SUCCESS(status, block_init_error);
    status = ee_key_init(&key, key_data);
    EE_GOTO_IF_NOT_SUCCESS(status, key_init_error);
    
    ee_number_init(&number);
    ee_subnumber_init(&subnumber);
    
    mpz_init(rho);
    mpz_init(delta);
    
    do {
        ee_size_t stats_len = (sigma + 1) * EE_ALPHABET_SIZE;
        status = ee_file_read_sdata(&statistics_data, stats_len, infile);
        if (EE_END_OF_FILE == status) {
            status = EE_SUCCESS;
            break;
        }
        
        ee_statistics_deserialize(&statistics, &statistics_data, sigma);
        status = ee_file_read_sdata(&subset_data, sigma + 4, infile);
        EE_BREAK_IF_NOT_SUCCESS(status);
        ee_subset_deserialize(&(subnumber.subset), sigma, &subset_data);
        ee_block_generate(&block, &statistics);
        status = ee_eval_rho(rho, &block, &statistics);
        EE_BREAK_IF_NOT_SUCCESS(status);
        mpz_cdiv_q(delta, deltas.deltas[deltas.sigma][0], rho);
        ee_eval_subnum_bit_length(&(subnumber.subnum_bit_length), delta,
                subnumber.subset);
        status = ee_file_read_sdata(&subnum_data, subnumber.subnum_bit_length,
                infile);
        EE_BREAK_IF_NOT_SUCCESS(status);
        ee_sdata_decrypt(&subnum_data, &key);
        ee_mpz_deserialize(subnumber.subnum, subnumber.subnum_bit_length,
                &subnum_data);
        ee_number_restore(&number, delta, &subnumber);
        status = ee_block_restore(&block, &statistics, rho, &deltas, &number);
        EE_BREAK_IF_NOT_SUCCESS(status);
        status = ee_file_write_block(outfile, &block);
        EE_BREAK_IF_NOT_SUCCESS(status);
        ee_sdata_clear(&subset_data);
        ee_sdata_clear(&subnum_data);
        ee_sdata_clear(&statistics_data);
    } while (1);
    
    mpz_clear(delta);
    mpz_clear(rho);
    
    ee_sdata_clear(&subset_data);
    ee_sdata_clear(&subnum_data);
    ee_sdata_clear(&statistics_data);
    
    ee_subnumber_deinit(&subnumber);
    ee_number_deinit(&number);
    
    ee_key_deinit(&key);
key_init_error:
    ee_block_deinit(&block);
block_init_error:
deltas_get_error:    
    ee_deltas_deinit(&deltas);
deltas_init_error:
    return status;
}
