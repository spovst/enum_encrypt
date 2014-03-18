#ifndef ARGS_H
#define	ARGS_H

#include "common.h"

typedef enum ee_mode_s {
    EE_MODE_ENCRYPT,
    EE_MODE_DECRYPT
} ee_mode_t;

typedef struct ee_args_s {
    ee_mode_t mode;
    ee_size_t sigma;
    const ee_char_t *key;
    const ee_char_t *input_file;
    const ee_char_t *output_file;
} ee_args_t;

ee_int_t
ee_args_parse(ee_args_t *args, int argc, char *argv[]);

#endif /* ARGS_H */
