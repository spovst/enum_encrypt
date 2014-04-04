#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "args.h"
#include "io.h"
#include "crypt.h"

static void
ee_print_error(ee_int_t code);

int
main(int argc, char *argv[])
{
    ee_args_t args;
    
    if (EE_SUCCESS != ee_args_parse(&args, argc, argv)) {
        return EE_FAILURE;
    }
    
    ee_int_t status = EE_SUCCESS;
    ee_file_t input, output;
    
    status = ee_file_open(&input, args.input_file, EE_MODE_READ);
    if (EE_SUCCESS != status) {
        switch (status) {
        case EE_FILE_NOT_EXISTS:
            fprintf(stderr, "%s: file '%s' does not exists\n", argv[0],
                    args.input_file);
            goto input_file_error;
            break;
        case EE_FILE_OPEN_FAILURE:
            fprintf(stderr, "%s: file '%s' opening error\n", argv[0],
                    args.input_file);
            goto input_file_error;
            break;
        default:
            fprintf(stderr, "%s: file '%s' opening unexpected error", argv[0],
                    args.input_file);
            goto input_file_error;
            break;
        }
    }
    
    status = ee_file_open(&output, args.output_file, EE_MODE_WRITE);
    if (EE_SUCCESS != status) {
        switch (status) {
        case EE_FILE_OPEN_FAILURE:
            fprintf(stderr, "%s: file '%s' opening error\n", argv[0],
                    args.output_file);
            goto output_file_error;
            break;
        default:
            fprintf(stderr, "%s: file '%s' opening unexpected error", argv[0],
                    args.output_file);
            goto output_file_error;
            break;
        }
    }
    
    switch (args.mode) {
    case EE_MODE_ENCRYPT:
        status = ee_encrypt(&output, &input, args.key, args.sigma);
        if (EE_SUCCESS != status) {
            ee_print_error(status);
        }
        
        break;
    case EE_MODE_DECRYPT:
        status = ee_decrypt(&output, &input, args.key, args.sigma);
        if (EE_SUCCESS != status) {
            ee_print_error(status);
        }
        
        break;
    }
    
    ee_file_close(&output);
output_file_error:
    ee_file_close(&input);
input_file_error:
    return 0;
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
