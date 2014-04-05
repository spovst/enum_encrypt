#ifndef IO_H
#define	IO_H

#include <stdio.h>

#include "common.h"
#include "bits.h"
#include "block.h"
#include "serializer.h"

#define EE_MODE_READ 1
#define EE_MODE_WRITE 2

typedef struct ee_file_s {
    FILE *file;
    ee_int_t mode;
    ee_byte_t *buffer;
    ee_size_t buffer_size;
    ee_bit_info_t bit_info;
    ee_int_t status;
} ee_file_t;

ee_int_t
ee_file_open(ee_file_t *file, const ee_char_t *name, ee_int_t mode);
void
ee_file_close(ee_file_t *file);

ee_int_t
ee_file_last_status(ee_file_t *file);

ee_int_t
ee_file_flush(ee_file_t *file);

ee_size_t
ee_file_read(ee_byte_t *bytes, ee_size_t number, ee_file_t *file);
ee_size_t
ee_file_write(ee_file_t *file, ee_byte_t *bytes, ee_size_t number);

ee_int_t
ee_file_read_bits(ee_byte_t *bytes, ee_size_t bits_number, ee_file_t *file);
ee_int_t
ee_file_write_bits(ee_file_t *file, ee_byte_t *bytes, ee_size_t bits_number);

ee_int_t
ee_file_read_byte_bits(ee_byte_t *byte, ee_size_t bits_number, ee_file_t *file);
ee_int_t
ee_file_write_byte_bits(ee_file_t *file, ee_byte_t byte, ee_size_t bits_number);

ee_int_t
ee_file_read_sdata(ee_sdata_t *sdata, ee_size_t bits_number, ee_file_t *file);
ee_int_t
ee_file_write_sdata(ee_file_t *file, ee_sdata_t *sdata);

ee_int_t
ee_file_read_block(ee_block_t *block, ee_file_t *file);
ee_int_t
ee_file_write_block(ee_file_t *file, ee_block_t *block);

ee_int_t
ee_file_read_message(ee_message_t *message, ee_file_t *file);
ee_int_t
ee_file_write_message(ee_file_t *file, ee_message_t *message);

#endif	/* IO_H */
