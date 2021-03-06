#include <stdlib.h>
#include <gmp.h>

#include "crypt.h"

#include "encryption.h"

#define EE_GOTO_IF_NOT_SUCCESS(status, label) \
        if (EE_SUCCESS != (status)) { \
            goto label; \
        }


#define EE_BREAK_IF(cond) \
        if ((cond)) { \
            break; \
        }

#define EE_BREAK_IF_NOT_SUCCESS(status) EE_BREAK_IF((EE_SUCCESS != (status)))

ee_int_t
ee_encrypt_source_list_s(ee_file_t *pub_outfile, ee_file_t *pri_outfile,
        ee_source_list_t *sources, ee_key_t *key, ee_size_t sigma);
ee_int_t
ee_encrypt_source_s(ee_file_t *pub_outfile, ee_file_t *pri_outfile,
        ee_source_t *source, ee_key_t *key, ee_size_t sigma, ee_size_t mu);
ee_int_t
ee_encrypt_source_chars_s(ee_file_t *pub_outfile, ee_file_t *pri_outfile,
        ee_source_t *source, ee_key_t *key, ee_size_t sigma);

ee_int_t
ee_decrypt_source_list_s(ee_source_list_t *sources, ee_file_t *pub_infile,
        ee_file_t *pri_infile, ee_key_t *key, ee_size_t sigma);
ee_int_t
ee_decrypt_source_s(ee_source_t *source, ee_file_t *pub_infile,
        ee_file_t *pri_infile, ee_key_t *key, ee_size_t sigma, ee_size_t mu);
ee_int_t
ee_decrypt_source_chars_s(ee_source_t *source, ee_file_t *pub_infile,
        ee_file_t *pri_infile, ee_size_t length, ee_key_t *key, ee_size_t sigma);

typedef struct ee_encrypt_source_context_s {
    ee_file_t *pub_outfile;
    ee_file_t *pri_outfile;
    ee_key_t *key;
    ee_size_t sigma;
    ee_size_t mu;
    ee_int_t status;
} ee_encrypt_source_context_t;

static ee_bool_t
ee_encrypt_source_handler_s(ee_source_t *source, void *context);

ee_int_t
ee_encrypt(ee_file_t *pub_outfile, ee_file_t *pri_outfile, ee_file_t *infile,
        ee_file_t *srcsfile, const ee_char_t *key_data, ee_size_t sigma,
        ee_size_t mu)
{
    ee_int_t status;

    ee_message_t message;
    ee_source_list_t sources;

    ee_key_t key;

    status = ee_key_init(&key, key_data);
    EE_GOTO_IF_NOT_SUCCESS(status, key_init_error);
    ee_source_list_init(&sources, mu);
    status = ee_file_read_message(&message, infile);
    EE_GOTO_IF_NOT_SUCCESS(status, message_read_error);
    status = ee_source_split(&sources, &message);
    EE_GOTO_IF_NOT_SUCCESS(status, source_split_error);
    status = ee_encrypt_source_list_s(pub_outfile, pri_outfile, &sources, &key,
            sigma);
    EE_GOTO_IF_NOT_SUCCESS(status, encrypt_source_error);
    if (NULL != srcsfile) {
        status = ee_file_dump_sources(srcsfile, &sources);
    }

encrypt_source_error:
source_split_error:
    ee_message_deinit(&message);
message_read_error:
    ee_source_list_deinit(&sources);
    ee_key_deinit(&key);
key_init_error:
    return status;
}

ee_int_t
ee_decrypt(ee_file_t *outfile, ee_file_t *pub_infile, ee_file_t *pri_infile,
        const ee_char_t *key_data, ee_size_t sigma, ee_size_t mu)
{
    ee_int_t status;

    ee_message_t message;
    ee_source_list_t sources;

    ee_key_t key;

    ee_size_t message_length;

    status = ee_key_init(&key, key_data);
    EE_GOTO_IF_NOT_SUCCESS(status, key_init_error);
    ee_source_list_init(&sources, mu);
    status = ee_decrypt_source_list_s(&sources, pub_infile, pri_infile, &key,
            sigma);
    EE_GOTO_IF_NOT_SUCCESS(status, decrypt_sources_error);
    message_length = ee_source_list_eval_message_length(&sources);
    status = ee_message_init(&message, message_length);
    EE_GOTO_IF_NOT_SUCCESS(status, message_init_error);
    status = ee_source_merge(&message, &sources);
    EE_GOTO_IF_NOT_SUCCESS(status, sources_merge_error);
    status = ee_file_write_message(outfile, &message);

sources_merge_error:
    ee_message_deinit(&message);
message_init_error:
decrypt_sources_error:
    ee_source_list_deinit(&sources);
    ee_key_deinit(&key);
key_init_error:
    return status;
}

ee_int_t
ee_encrypt_source_list_s(ee_file_t *pub_outfile, ee_file_t *pri_outfile,
        ee_source_list_t *sources, ee_key_t *key, ee_size_t sigma)
{
    ee_encrypt_source_context_t context;

    context.pub_outfile = pub_outfile;
    context.pri_outfile = pri_outfile;
    context.key = key;
    context.sigma = sigma;
    context.mu = sources->mu;

    ee_source_list_traverse(sources, ee_encrypt_source_handler_s, &context);

    return context.status;
}

ee_int_t
ee_encrypt_source_s(ee_file_t *pub_outfile, ee_file_t *pri_outfile,
        ee_source_t *source, ee_key_t *key, ee_size_t sigma, ee_size_t mu)
{
    ee_int_t status;

    ee_sdata_t si_sdata;

    status = ee_source_info_serialize(&si_sdata, source, mu);
    EE_GOTO_IF_NOT_SUCCESS(status, si_sdata_serialize_error);
    status = ee_file_write_sdata(pub_outfile, &si_sdata);
    EE_GOTO_IF_NOT_SUCCESS(status, si_sdata_write_error);
    if (1 != source->length) {
        status = ee_encrypt_source_chars_s(pub_outfile, pri_outfile, source,
                key, sigma);
    }

si_sdata_write_error:
    ee_sdata_clear(&si_sdata);
si_sdata_serialize_error:
    return status;
}

ee_int_t
ee_encrypt_source_chars_s(ee_file_t *pub_outfile, ee_file_t *pri_outfile,
        ee_source_t *source, ee_key_t *key, ee_size_t sigma)
{
    ee_size_t status;
    ee_int_t block_status;

    ee_block_t block;
    ee_statistics_t statistics;
    ee_number_t number;
    ee_subnumber_t subnumber;

    ee_sdata_t statistics_data = { .bytes = NULL, .bits_number = 0 };
    ee_sdata_t subnum_data = { .bytes = NULL, .bits_number = 0 };
    ee_sdata_t subset_data = { .bytes = NULL, .bits_number = 0 };

    ee_size_t offset;

    status = ee_block_init(&block, sigma);
    EE_GOTO_IF_NOT_SUCCESS(status, block_init_error);

    ee_number_init(&number);
    ee_subnumber_init(&subnumber);

    offset = 0;
    do {
        block_status = ee_block_from_source(&block, source, offset);
        EE_BREAK_IF(0 == block.length);
        offset += block.length;
        ee_statistics_gather(&statistics, &block);
        status = ee_number_eval(&number, &block, &statistics);
        EE_BREAK_IF_NOT_SUCCESS(status);
        ee_subnumber_eval(&subnumber, &number);
        status = ee_mpz_serialize(&subnum_data, subnumber.subnum,
                subnumber.subnum_bit_length);
        EE_BREAK_IF_NOT_SUCCESS(status);
        ee_sdata_encrypt(&subnum_data, key);
        status = ee_statistics_serialize(&statistics_data, &statistics, sigma);
        EE_BREAK_IF_NOT_SUCCESS(status);
        status = ee_subset_serialize(&subset_data, subnumber.subset, sigma);
        EE_BREAK_IF_NOT_SUCCESS(status);
        status = ee_file_write_sdata(pub_outfile, &statistics_data);
        EE_BREAK_IF_NOT_SUCCESS(status);
        status = ee_file_write_sdata(pub_outfile, &subset_data);
        EE_BREAK_IF_NOT_SUCCESS(status);
        status = ee_file_write_sdata(pri_outfile, &subnum_data);
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

    ee_block_deinit(&block);
block_init_error:
    return status;
}

ee_int_t
ee_decrypt_source_list_s(ee_source_list_t *sources, ee_file_t *pub_infile,
        ee_file_t *pri_infile, ee_key_t *key, ee_size_t sigma)
{
    ee_int_t status;

    do {
        ee_source_t *source;
        source = calloc(1, sizeof(*source));
        if (NULL == source) {
            status = EE_ALLOC_FAILURE;
            break;
        }

        status = ee_source_init(source, NULL, sources->mu);
        if (EE_SUCCESS != status) {
            free(source);
            break;
        }

        status = ee_decrypt_source_s(source, pub_infile, pri_infile, key, sigma,
                sources->mu);
        if (EE_SUCCESS != status) {
            ee_source_deinit(source);
            free(source);
            break;
        }

        status = ee_source_list_insert(sources, source);
        if (EE_SUCCESS != status) {
            ee_source_deinit(source);
            free(source);
            break;
        }
    } while (1);

    if (EE_END_OF_FILE == status) {
        status = EE_SUCCESS;
    }

    return status;
}

ee_int_t
ee_decrypt_source_s(ee_source_t *source, ee_file_t *pub_infile,
        ee_file_t *pri_infile, ee_key_t *key, ee_size_t sigma, ee_size_t mu)
{
    ee_int_t status;

    ee_sdata_t si_sdata;
    ee_size_t si_bit_length = (mu + 1 + 4) * EE_BITS_IN_BYTE;

    ee_char_t last_char;
    ee_size_t length;

    status = ee_file_read_sdata(&si_sdata, si_bit_length, pub_infile);
    EE_GOTO_IF_NOT_SUCCESS(status, si_sdata_read_error);
    ee_source_info_deserialize(source, &last_char, &length, &si_sdata, mu);
    if (1 != length) {
        status = ee_decrypt_source_chars_s(source, pub_infile, pri_infile,
                length, key, sigma);
        EE_GOTO_IF_NOT_SUCCESS(status, decrypt_source_error);
    }

    status = ee_source_append_char(source, last_char);

decrypt_source_error:
si_sdata_read_error:
    ee_sdata_clear(&si_sdata);
    return status;
}

ee_int_t
ee_decrypt_source_chars_s(ee_source_t *source, ee_file_t *pub_infile,
        ee_file_t *pri_infile, ee_size_t length, ee_key_t *key, ee_size_t sigma)
{
    ee_int_t status;

    ee_block_t block;
    ee_statistics_t statistics;
    ee_number_t number;
    ee_subnumber_t subnumber;

    ee_sdata_t statistics_data = { .bytes = NULL, .bits_number = 0 };
    ee_sdata_t subnum_data = { .bytes = NULL, .bits_number = 0 };
    ee_sdata_t subset_data = { .bytes = NULL, .bits_number = 0 };

    mpz_t rho = EE_MPZ_NULL;
    mpz_t delta = EE_MPZ_NULL;

    ee_size_t inc_length;

    status = ee_block_init(&block, sigma);
    EE_GOTO_IF_NOT_SUCCESS(status, block_init_error);

    ee_number_init(&number);
    ee_subnumber_init(&subnumber);

    mpz_init(rho);
    mpz_init(delta);

    inc_length = 0;
    do {
        ee_size_t stats_len = (sigma + 1) * EE_ALPHABET_SIZE;
        status = ee_file_read_sdata(&statistics_data, stats_len, pub_infile);
        if (EE_END_OF_FILE == status) {
            status = EE_SUCCESS;
            break;
        }

        ee_statistics_deserialize(&statistics, &statistics_data, sigma);
        status = ee_file_read_sdata(&subset_data, sigma + 4, pub_infile);
        EE_BREAK_IF_NOT_SUCCESS(status);
        ee_subset_deserialize(&(subnumber.subset), &subset_data);
        ee_block_generate(&block, &statistics);
        status = ee_eval_rho(rho, &block, &statistics);
        EE_BREAK_IF_NOT_SUCCESS(status);
        status = ee_eval_delta(delta, rho, &block, &statistics);
        EE_BREAK_IF_NOT_SUCCESS(status);
        ee_eval_subnum_bit_length(&(subnumber.subnum_bit_length), delta,
                subnumber.subset);
        status = ee_file_read_sdata(&subnum_data, subnumber.subnum_bit_length,
                pri_infile);
        EE_BREAK_IF_NOT_SUCCESS(status);
        ee_sdata_decrypt(&subnum_data, key);
        ee_mpz_deserialize(subnumber.subnum, subnumber.subnum_bit_length,
                &subnum_data);
        ee_number_restore(&number, delta, &subnumber);
        status = ee_block_restore(&block, &statistics, rho, &number);
        EE_BREAK_IF_NOT_SUCCESS(status);
        status = ee_source_append_block(source, &block);
        EE_BREAK_IF_NOT_SUCCESS(status);
        inc_length += block.length;
        ee_sdata_clear(&subset_data);
        ee_sdata_clear(&subnum_data);
        ee_sdata_clear(&statistics_data);
    } while (inc_length < length - 1);

    mpz_clear(delta);
    mpz_clear(rho);

    ee_sdata_clear(&subset_data);
    ee_sdata_clear(&subnum_data);
    ee_sdata_clear(&statistics_data);

    ee_subnumber_deinit(&subnumber);
    ee_number_deinit(&number);

    ee_block_deinit(&block);
block_init_error:
    return status;
}

static ee_bool_t
ee_encrypt_source_handler_s(ee_source_t *source, void *context)
{
    ee_encrypt_source_context_t *ctx = context;

    ctx->status = ee_encrypt_source_s(ctx->pub_outfile, ctx->pri_outfile,
            source, ctx->key, ctx->sigma, ctx->mu);

    return (EE_SUCCESS == ctx->status) ? EE_TRUE : EE_FALSE;
}
