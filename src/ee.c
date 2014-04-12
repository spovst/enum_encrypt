#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "args.h"
#include "io.h"
#include "crypt.h"

#define EE_PUB_EXT ".pub"
#define EE_PRI_EXT ".pri"

static ee_int_t
ee_do_encrypt(ee_args_t *args, const char *pname);
static ee_int_t
ee_do_decrypt(ee_args_t *args, const char *pname);

static ee_int_t
ee_open_pub_pri(ee_file_t *pub_file, ee_file_t *pri_file, const ee_char_t *name,
        ee_int_t mode, const ee_char_t *pname);
static ee_int_t
ee_do_open(ee_file_t *file, const ee_char_t *name, ee_int_t mode,
        const ee_char_t *pname);

static void
ee_print_error(ee_int_t code);

int
main(int argc, char *argv[])
{
    ee_int_t status = EE_SUCCESS;
    ee_args_t args;
    
    if (EE_SUCCESS != ee_args_parse(&args, argc, argv)) {
        return EE_FAILURE;
    }
    
    switch (args.mode) {
    case EE_MODE_ENCRYPT:
        status = ee_do_encrypt(&args, argv[0]);
        break;
    case EE_MODE_DECRYPT:
        status = ee_do_decrypt(&args, argv[0]);
        break;
    }

    return status;
}

static ee_int_t
ee_do_encrypt(ee_args_t *args, const char *pname)
{
    ee_int_t status = EE_SUCCESS;
    ee_file_t input;
    ee_file_t pub_output, pri_output;
    ee_file_t *pub_output_ptr = NULL, *pri_output_ptr = NULL;
    ee_file_t sources;
    ee_file_t *sources_ptr = NULL;
    
    status = ee_do_open(&input, args->input_file, EE_MODE_READ, pname);
    if (EE_SUCCESS != status) {
        goto input_open_error;
    }
    
    if (EE_TRUE == args->part) {
        pub_output_ptr = &pub_output;
        pri_output_ptr = &pri_output;
        status = ee_open_pub_pri(pub_output_ptr, pri_output_ptr,
                args->output_file, EE_MODE_WRITE, pname);
        if (EE_SUCCESS != status) {
            goto output_open_error;
        }
    } else {
        pub_output_ptr = &pub_output;
        pri_output_ptr = &pub_output;
        status = ee_do_open(pub_output_ptr, args->output_file, EE_MODE_WRITE,
                pname);
        if (EE_SUCCESS != status) {
            goto output_open_error;
        }
    }
    
    if (EE_TRUE == args->dump_sources) {
        status = ee_do_open(&sources, "sources.dump", EE_MODE_WRITE, pname);
        if (EE_SUCCESS != status) {
            goto sources_open_error;
        }
        
        sources_ptr = &sources;
    }
    
    status = ee_encrypt(pub_output_ptr, pri_output_ptr, &input, sources_ptr,
            args->key, args->sigma, args->mu);
    if (EE_SUCCESS != status) {
        ee_print_error(status);
    }
    
    if (EE_TRUE == args->dump_sources) {
        ee_file_close(&sources);
    }
sources_open_error:
    if (EE_TRUE == args->part) {
        ee_file_close(&pub_output);
        ee_file_close(&pri_output);
    } else {
        ee_file_close(&pub_output);
    }
output_open_error:
    ee_file_close(&input);
input_open_error:
    return status;
}

static ee_int_t
ee_do_decrypt(ee_args_t *args, const char *pname)
{
    ee_int_t status = EE_SUCCESS;
    ee_file_t pub_input, pri_input;
    ee_file_t *pub_input_ptr = NULL, *pri_input_ptr = NULL;
    ee_file_t output;
    
    status = ee_do_open(&output, args->output_file, EE_MODE_WRITE, pname);
    if (EE_SUCCESS != status) {
        goto output_open_error;
    }
    
    if (EE_TRUE == args->part) {
        pub_input_ptr = &pub_input;
        pri_input_ptr = &pri_input;
        status = ee_open_pub_pri(pub_input_ptr, pri_input_ptr,
                args->input_file, EE_MODE_READ, pname);
        if (EE_SUCCESS != status) {
            goto input_open_error;
        }
    } else {
        pub_input_ptr = &pub_input;
        pri_input_ptr = &pub_input;
        status = ee_do_open(pub_input_ptr, args->input_file, EE_MODE_READ,
                pname);
        if (EE_SUCCESS != status) {
            goto input_open_error;
        }
    }
    
    status = ee_decrypt(&output, pub_input_ptr, pri_input_ptr, args->key,
            args->sigma, args->mu);
    if (EE_SUCCESS != status) {
        ee_print_error(status);
    }
    
    if (EE_TRUE == args->part) {
        ee_file_close(&pub_input);
        ee_file_close(&pri_input);
    } else {
        ee_file_close(&pub_input);
    }
input_open_error:
    ee_file_close(&output);
output_open_error:
    return status;
}

static ee_int_t
ee_open_pub_pri(ee_file_t *pub_file, ee_file_t *pri_file, const ee_char_t *name,
        ee_int_t mode, const ee_char_t *pname)
{
    ee_int_t status = EE_SUCCESS;
    ee_char_t *pub_name = NULL, *pri_name = NULL;
    ee_size_t pub_name_len, pri_name_len;

    pub_name_len = strlen(name) + strlen(EE_PUB_EXT);
    pri_name_len = strlen(name) + strlen(EE_PRI_EXT);

    pub_name = calloc(pub_name_len + 1, sizeof(*pub_name));
    if (NULL == pub_name) {
        status = EE_ALLOC_FAILURE;
        goto pub_alloc_error;
    }

    pri_name = calloc(pri_name_len + 1, sizeof(*pri_name));
    if (NULL == pub_name) {
        status = EE_ALLOC_FAILURE;
        goto pri_alloc_error;
    }

    strcpy(pub_name, name);
    strcat(pub_name, EE_PUB_EXT);
    strcpy(pri_name, name);
    strcat(pri_name, EE_PRI_EXT);

    status = ee_do_open(pub_file, pub_name, mode, pname);
    if (EE_SUCCESS != status) {
        goto pub_open_error;
    }
    
    status = ee_do_open(pri_file, pri_name, mode, pname);
    if (EE_SUCCESS != status) {
        ee_file_close(pub_file);
    }

pub_open_error:
    free(pri_name);
pri_alloc_error:
    free(pub_name);
pub_alloc_error:
    return status;
}

static ee_int_t
ee_do_open(ee_file_t *file, const ee_char_t *name, ee_int_t mode,
        const ee_char_t *pname)
{
    ee_int_t status = EE_SUCCESS;
    
    status = ee_file_open(file, name, mode);
    if (EE_SUCCESS != status) {
        switch (status) {
        case EE_FILE_NOT_EXISTS:
            fprintf(stderr, "%s: file '%s' does not exists\n", pname, name);
            break;
        case EE_FILE_OPEN_FAILURE:
            fprintf(stderr, "%s: file '%s' opening error\n", pname, name);
            break;
        default:
            fprintf(stderr, "%s: file '%s' opening unexpected error", pname,
                    name);
            break;
        }
    }
    
    return status;
}

static void
ee_print_error(ee_int_t code)
{
    switch (code) {
    case EE_FAILURE:
        fprintf(stderr, "unexpected error\n");
        break;
    case EE_ALLOC_FAILURE:
        fprintf(stderr, "memory allocate error\n");
        break;
    case EE_FILE_NOT_EXISTS:
        fprintf(stderr, "file does not exists\n");
        break;
    case EE_FILE_OPEN_FAILURE:
        fprintf(stderr, "file opening error\n");
        break;
    case EE_FILE_READ_FAILURE:
        fprintf(stderr, "file reading error\n");
        break;
    case EE_FILE_WRITE_FAILURE:
        fprintf(stderr, "file writing error\n");
        break;
    case EE_INVALID_MODE:
        fprintf(stderr, "invalid file opening mode\n");
        break;
    case EE_INCORRECT_MODE:
        fprintf(stderr, "incorrect file mode (requested operation is not "
                "allowed)\n");
        break;
    default:
        fprintf(stderr, "unknown error code: %lu\n", code);
        break;
    }
}
