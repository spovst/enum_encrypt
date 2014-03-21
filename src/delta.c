#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <gmp.h>

#include "common.h"
#include "delta.h"
#include "util.h"

#define EE_MAX_FN_LEN 64
#define DELTA_FN_SUFFIX ".delta"

static void
ee_deltas_eval_s(ee_deltas_t *deltas);
static ee_int_t
ee_deltas_read_s(ee_deltas_t *deltas, FILE *idfile);
static ee_int_t
ee_deltas_write_s(FILE *odfile, ee_deltas_t *deltas);

ee_int_t
ee_deltas_init(ee_deltas_t *deltas, ee_size_t sigma)
{
    ee_int_t status = EE_SUCCESS;
    ee_size_t bsize = 1 << sigma;
    ee_size_t rows = sigma + 1;
    
    deltas->sigma = sigma;
    deltas->deltas = calloc(rows, sizeof(*(deltas->deltas)));
    if (NULL == deltas->deltas) {
        status = EE_ALLOC_FAILURE;
        goto deltas_calloc_error;
    }
    
    for (ee_size_t i = 0; i < rows; ++i) {
        ee_size_t cols = bsize >> i;
        deltas->deltas[i] = calloc(cols, sizeof(**(deltas->deltas)));
        if (NULL == deltas->deltas[i]) {
            status = EE_ALLOC_FAILURE;
            goto deltas_i_calloc_error;
        }
    }
    
    for (ee_size_t i = 0; i < rows; ++i) {
        ee_size_t cols = bsize >> i;
        for (ee_size_t j = 0; j < cols; ++j) {
            mpz_init(deltas->deltas[i][j]);
        }
    }
    
    return status;
    
deltas_i_calloc_error:
    for (ee_size_t i = 0; i < rows; ++i) {
        free(deltas->deltas[i]);
    }

    free(deltas->deltas);
deltas_calloc_error:
    return status;
}

void
ee_deltas_deinit(ee_deltas_t *deltas)
{
    ee_size_t bsize = 1 << deltas->sigma;
    ee_size_t rows = deltas->sigma + 1;
    
    for (ee_size_t i = 0; i < rows; ++i) {
        ee_size_t cols = bsize >> i;
        for (ee_size_t j = 0; j < cols; ++j) {
            mpz_clear(deltas->deltas[i][j]);
        }
        
        free(deltas->deltas[i]);
    }
    
    free(deltas->deltas);
    ee_memset(deltas, 0, sizeof(*deltas));
}

ee_int_t
ee_deltas_get(ee_deltas_t *deltas)
{
    ee_char_t dfname[EE_MAX_FN_LEN];
    FILE *dfile = NULL;
    ee_int_t status = EE_SUCCESS;
    
    snprintf(dfname, EE_MAX_FN_LEN, "%u%s", deltas->sigma, DELTA_FN_SUFFIX);
	if (EE_FALSE == ee_is_file_exists(dfname)) {
        dfile = fopen(dfname, "wb");
        if (NULL == dfile) {
            status = EE_FILE_OPEN_FAILURE;
            goto fopen_error;
        }
        
        ee_deltas_eval_s(deltas);
        status = ee_deltas_write_s(dfile, deltas);
        if (EE_SUCCESS != status) {
            goto file_error;
        }
    } else {
        dfile = fopen(dfname, "rb");
        if (NULL == dfile) {
            status = EE_FILE_OPEN_FAILURE;
            goto fopen_error;
        }
        
        status = ee_deltas_read_s(deltas, dfile);
        if (EE_SUCCESS != status) {
            goto file_error;
        }
    }
    
file_error:
    fclose(dfile);
fopen_error:
    return status;
}

static void
ee_deltas_eval_s(ee_deltas_t *deltas)
{
    ee_size_t bsize = 1 << deltas->sigma;
    ee_size_t rows = deltas->sigma + 1;
    
    for (ee_size_t i = 0; i < bsize; ++i) {
        mpz_set_ui(deltas->deltas[0][i], bsize - i);
    }
    
    for (ee_size_t i = 1; i < rows; ++i) {
        ee_size_t cols = bsize >> i;
        for (ee_size_t j = 1; j <= cols; ++j) {
            mpz_mul(deltas->deltas[i][j - 1],
                    deltas->deltas[i - 1][2 * j - 2],
                    deltas->deltas[i - 1][2 * j - 1]);
        }
    }
}

static ee_int_t
ee_deltas_read_s(ee_deltas_t *deltas, FILE *idfile)
{
    ee_int_t status = EE_SUCCESS;
    size_t readed_bytes;
    ee_size_t bsize = 1 << deltas->sigma;
    ee_size_t rows = deltas->sigma + 1;
    
    for (ee_size_t i = 0; i < rows; ++i) {
        ee_size_t cols = bsize >> i;
        for (ee_size_t j = 0; j < cols; ++j) {
            readed_bytes = mpz_inp_raw(deltas->deltas[i][j], idfile);
            if (0 == readed_bytes) {
                status = EE_FILE_READ_FAILURE;
                goto read_error;
            }
        }
    }
    
read_error:
    return status;
}

static ee_int_t
ee_deltas_write_s(FILE *odfile, ee_deltas_t *deltas)
{
    ee_int_t status = EE_SUCCESS;
    size_t written_bytes;
    ee_size_t bsize = 1 << deltas->sigma;
    ee_size_t rows = deltas->sigma + 1;
    
    for (ee_size_t i = 0; i < rows; ++i) {
        ee_size_t cols = bsize >> i;
        for (ee_size_t j = 0; j < cols; ++j) {
            written_bytes = mpz_out_raw(odfile, deltas->deltas[i][j]);
            if (0 == written_bytes) {
                status = EE_FILE_WRITE_FAILURE;
                goto write_error;
            }
        }
    }
    
write_error:
    return status;
}
