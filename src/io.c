#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "io.h"

#include "common.h"
#include "bits.h"
#include "block.h"
#include "util.h"

#define EE_IO_BUFFER_SIZE (64 * 1024)

static const ee_char_t *
ee_file_smode_build_s(ee_int_t mode);

static void
ee_file_buffer_fill_s(ee_file_t *file);

static ee_size_t
ee_file_read_aligned_s(ee_byte_t *bytes, ee_size_t count, ee_file_t *file);
static ee_size_t
ee_file_read_not_aligned_s(ee_byte_t *bytes, ee_size_t count, ee_file_t *file);

static ee_size_t
ee_file_write_aligned_s(ee_file_t *file, ee_byte_t *bytes, ee_size_t count);
static ee_size_t
ee_file_write_not_aligned_s(ee_file_t *file, ee_byte_t *bytes, ee_size_t count);

static ee_size_t
ee_file_avail_bits_number_eval_s(ee_file_t *file);

static void
ee_file_copy_byte_bits_to_s(ee_byte_t *byte, ee_int_t start, ee_int_t end,
        ee_file_t *file);
static void
ee_file_copy_byte_bits_from_s(ee_file_t *file, ee_int_t start, ee_int_t end,
        ee_byte_t byte);

ee_int_t
ee_file_open(ee_file_t *file, const ee_char_t *name, ee_int_t mode)
{
    ee_int_t status = EE_SUCCESS;
    const ee_char_t *smode = ee_file_smode_build_s(mode);
    
    if (NULL == smode)  {
        status = EE_INVALID_MODE;
        goto build_smode_error;
    }
    
    if ((EE_MODE_READ == mode) && (0 != access(name, F_OK))) {
        status = EE_FILE_NOT_EXISTS;
        goto file_not_exists_error;
    }
    
    file->file = fopen(name, smode);
    if (NULL == file->file) {
        status = EE_FILE_OPEN_FAILURE;
        goto file_open_error;
    }
    
    file->buffer = calloc(EE_IO_BUFFER_SIZE, sizeof(ee_byte_t));
    if (NULL == file->buffer) {
        status = EE_ALLOC_FAILURE;
        goto calloc_error;
    }
    
    file->mode = mode;
    file->buffer_size = ((EE_MODE_WRITE == mode) ? EE_IO_BUFFER_SIZE : 0);
    file->bit_info.current_bit = EE_BITS_IN_BYTE - 1;
    file->bit_info.current_byte = 0;
    file->status = EE_SUCCESS;
    
calloc_error:
    if (EE_SUCCESS != status) {
        fclose(file->file);
    }
file_open_error:
file_not_exists_error:
build_smode_error:
    return status;
}

void
ee_file_close(ee_file_t *file)
{
    ee_file_flush(file);
    fclose(file->file);
    free(file->buffer);
    ee_memset(file, 0, sizeof(*file));
}

ee_int_t
ee_file_last_status(ee_file_t *file)
{
    return file->status;
}

ee_int_t
ee_file_flush(ee_file_t *file)
{
    if (EE_MODE_WRITE != file->mode) {
        file->status = EE_INCORRECT_MODE;
    } else {
        ee_size_t bytes_number = file->bit_info.current_byte + 1;
        if (EE_BITS_IN_BYTE - 1 == file->bit_info.current_bit) {
            bytes_number -= 1;
        }
        
        ee_size_t wcount = fwrite(file->buffer, sizeof(ee_byte_t), bytes_number,
                file->file);
        if (wcount != bytes_number) {
            if (0 != ferror(file->file)) {
                file->status = EE_FILE_WRITE_FAILURE;
            } else {
                file->status = EE_FAILURE;
            }
        }

        file->buffer_size = EE_IO_BUFFER_SIZE;
        file->bit_info.current_bit = EE_BITS_IN_BYTE - 1;
        file->bit_info.current_byte = 0;
    }
    
    return file->status;
}

ee_size_t
ee_file_read(ee_byte_t *bytes, ee_size_t number, ee_file_t *file)
{
    ee_size_t result;
    
    file->status = EE_SUCCESS;   
    if (EE_MODE_READ != file->mode) {
        file->status = EE_INCORRECT_MODE;
        result = 0;
    } else {
        if ((EE_BITS_IN_BYTE - 1) == file->bit_info.current_bit) {
            result = ee_file_read_aligned_s(bytes, number, file);
        } else {
            result = ee_file_read_not_aligned_s(bytes, number, file);
        }
    }
    
    return result;
}

ee_size_t
ee_file_write(ee_file_t *file, ee_byte_t *bytes, ee_size_t number)
{
    ee_size_t result;

    file->status = EE_SUCCESS;
    if (EE_MODE_WRITE != file->mode) {
        file->status = EE_INCORRECT_MODE;
        result = 0;
    } else {
        if ((EE_BITS_IN_BYTE - 1) == file->bit_info.current_bit) {
            result = ee_file_write_aligned_s(file, bytes, number);
        } else {
            result = ee_file_write_not_aligned_s(file, bytes, number);
        }
    }
    
    return result;
}

ee_int_t
ee_file_read_bits(ee_byte_t *bytes, ee_size_t bits_number, ee_file_t *file)
{
    ee_int_t status = EE_SUCCESS;
    ee_size_t bytes_number = EE_EVAL_BYTES_NUMBER(bits_number);
    ee_size_t rem = bits_number % EE_BITS_IN_BYTE;
    
    if (0 != rem) {
        bytes_number -= 1;
    }
    
    if (ee_file_read(bytes, bytes_number, file) != bytes_number) {
        status = file->status;
    } else {
        if (0 != rem) {
            status = ee_file_read_byte_bits(bytes + bytes_number, rem, file);
        }
    }
    
    return status;
}

ee_int_t
ee_file_write_bits(ee_file_t *file, ee_byte_t *bytes, ee_size_t bits_number)
{
    ee_int_t status = EE_SUCCESS;
    ee_size_t bytes_number = EE_EVAL_BYTES_NUMBER(bits_number);
    ee_size_t rem = bits_number % EE_BITS_IN_BYTE;
    
    if (0 != rem) {
        bytes_number -= 1;
    }
    
    if (ee_file_write(file, bytes, bytes_number) != bytes_number) {
        status = file->status;
    } else {
        if (0 != rem) {
            status = ee_file_write_byte_bits(file, bytes[bytes_number], rem);
        }
    }
    
    return status;
}

ee_int_t
ee_file_read_byte_bits(ee_byte_t *byte, ee_size_t bits_number, ee_file_t *file)
{
    ee_int_t status = EE_SUCCESS;
    ee_size_t avail_bits_number = ee_file_avail_bits_number_eval_s(file);
    
    if (EE_BITS_IN_BYTE < bits_number) {
        bits_number = EE_BITS_IN_BYTE;
    }

    *byte = 0;
    if (bits_number <= avail_bits_number) {
        ee_file_copy_byte_bits_to_s(byte, bits_number, 0, file);
    } else {
        ee_size_t middle = bits_number - avail_bits_number;
        ee_file_copy_byte_bits_to_s(byte, bits_number, middle, file);
        ee_file_buffer_fill_s(file);
        if (EE_IO_BUFFER_SIZE != file->buffer_size) {
            if (0 != ferror(file->file)) {
                status = file->status = EE_FILE_READ_FAILURE;
                goto end;
            } else if (0 == file->buffer_size) {
                status = file->status = EE_END_OF_FILE;
                goto end;
            }
        }
        
        ee_file_copy_byte_bits_to_s(byte, middle, 0, file);
    }
    
end:
    return status;
}

ee_int_t
ee_file_write_byte_bits(ee_file_t *file, ee_byte_t byte, ee_size_t bits_number)
{
    ee_int_t status = EE_SUCCESS;
    ee_size_t avail_bits_number = ee_file_avail_bits_number_eval_s(file);
    
    if (EE_BITS_IN_BYTE < bits_number) {
        bits_number = EE_BITS_IN_BYTE;
    }
    
    if (bits_number <= avail_bits_number) {
        ee_file_copy_byte_bits_from_s(file, bits_number, 0, byte);
    } else {
        ee_size_t middle = bits_number - avail_bits_number;
        ee_file_copy_byte_bits_from_s(file, bits_number, middle, byte);
        status = ee_file_flush(file);
        if (EE_SUCCESS != status) {
            goto end;
        }
        
        ee_file_copy_byte_bits_from_s(file, middle, 0, byte);
    }
    
end:
    return status;
}

ee_int_t
ee_file_read_sdata(ee_sdata_t *sdata, ee_size_t bits_number, ee_file_t *file)
{
    ee_int_t status = EE_SUCCESS;
    ee_size_t bytes_number = EE_EVAL_BYTES_NUMBER(bits_number);
    
    sdata->bits_number = bits_number;
    sdata->bytes = calloc(bytes_number, sizeof(ee_byte_t));
    if (NULL == sdata->bytes) {
        status = EE_ALLOC_FAILURE;
    } else {
        status = ee_file_read_bits(sdata->bytes, sdata->bits_number, file);
    }
    
    return status;
}

ee_int_t
ee_file_write_sdata(ee_file_t *file, ee_sdata_t *sdata)
{
    return ee_file_write_bits(file, sdata->bytes, sdata->bits_number);
}

ee_int_t
ee_file_read_block(ee_block_t *block, ee_file_t *file)
{
    ee_int_t status = EE_SUCCESS;
    
    ee_memset(block->chars, 0, block->size);
    block->length = ee_file_read(block->chars, block->size, file);
    if (block->length != block->size) {
        status = EE_FINAL_BLOCK;
    }
    
    return status;
}

ee_int_t
ee_file_write_block(ee_file_t *file, ee_block_t *block)
{
    ee_int_t status = EE_SUCCESS;
    
    if (block->length != ee_file_write(file, block->chars, block->length)) {
        status = file->status;
    }
    
    return status;
}

static const ee_char_t *
ee_file_smode_build_s(ee_int_t mode)
{
    const ee_char_t *smode;
    
    if (EE_MODE_READ == mode) {
        smode = "rb";
    } else if (EE_MODE_WRITE == mode) {
        smode = "wb";
    } else {
        smode = NULL;
    }
    
    return smode;
}

static void
ee_file_buffer_fill_s(ee_file_t *file)
{
    file->buffer_size = fread(file->buffer, sizeof(ee_byte_t),
            EE_IO_BUFFER_SIZE, file->file);
    file->bit_info.current_bit = EE_BITS_IN_BYTE - 1;
    file->bit_info.current_byte = 0;
}

static ee_size_t
ee_file_read_aligned_s(ee_byte_t *bytes, ee_size_t count, ee_file_t *file)
{
    ee_size_t result = 0;
    
    while (result < count) {
        ee_size_t avail;
        if (file->bit_info.current_byte < file->buffer_size) {
            avail = file->buffer_size - file->bit_info.current_byte;
        } else {
            avail = file->bit_info.current_byte - file->buffer_size;
        }
        
        ee_size_t rem = count - result;
        if (rem < avail) {
            memcpy(bytes, file->buffer + file->bit_info.current_byte, rem);
            file->bit_info.current_byte += rem;
            result += rem;
        } else {
            memcpy(bytes, file->buffer + file->bit_info.current_byte, avail);
            file->bit_info.current_byte = 0;
            bytes += avail;
            result += avail;
            ee_file_buffer_fill_s(file);
            if (EE_IO_BUFFER_SIZE != file->buffer_size) {
                if (file->buffer_size < (rem - avail)) {
                    memcpy(bytes, file->buffer, file->buffer_size);
                    result += file->buffer_size;
                    if (0 != ferror(file->file)) {
                        file->status = EE_FILE_READ_FAILURE;
                    } else {
                        file->status = EE_END_OF_FILE;
                    }
                    
                    goto end;
                }
            }
        }
    }
    
end:
    return result;
}

static ee_size_t
ee_file_read_not_aligned_s(ee_byte_t *bytes, ee_size_t count, ee_file_t *file)
{
    ee_size_t i;
    
    for (i = 0; i < count; ++i) {
        file->status = ee_file_read_byte_bits(bytes + i, EE_BITS_IN_BYTE, file);
        if (EE_SUCCESS != file->status) {
            break;
        }
    }
    
    return i;
}

static ee_size_t
ee_file_write_aligned_s(ee_file_t *file, ee_byte_t *bytes, ee_size_t count)
{
    ee_size_t result = 0;
    
    while (result < count) {
        ee_size_t avail = file->buffer_size - file->bit_info.current_byte;
        ee_size_t rem = count - result;
        if (rem < avail) {
            memcpy(file->buffer + file->bit_info.current_byte, bytes, rem);
            file->bit_info.current_byte += rem;
            result += rem;
        } else {
            memcpy(file->buffer + file->bit_info.current_byte, bytes, avail);
            file->bit_info.current_byte += avail;
            bytes += avail;
            result += avail;
            if (EE_SUCCESS != ee_file_flush(file)) {
                break;
            }
        }
    }
    
    return result;
}

static ee_size_t
ee_file_write_not_aligned_s(ee_file_t *file, ee_byte_t *bytes, ee_size_t count)
{
    ee_size_t i;
    
    for (i = 0; i < count; ++i) {
        file->status = ee_file_write_byte_bits(file, bytes[i], EE_BITS_IN_BYTE);
        if (EE_SUCCESS != file->status) {
            break;
        }
    }
    
    return i;
}

static ee_size_t
ee_file_avail_bits_number_eval_s(ee_file_t *file)
{
    ee_size_t bs = file->buffer_size;
    ee_size_t cby = file->bit_info.current_byte;
    ee_size_t cbi = file->bit_info.current_bit;
    
    return (bs - cby) * EE_BITS_IN_BYTE - (EE_BITS_IN_BYTE - (cbi + 1));
}

static void
ee_file_copy_byte_bits_to_s(ee_byte_t *byte, ee_int_t start, ee_int_t end,
        ee_file_t *file)
{
    for (ee_size_t i = start; i > end; --i) {
        ee_byte_t value = file->buffer[file->bit_info.current_byte];
        ee_size_t bit = ee_bit_get(value, file->bit_info.current_bit);
        *byte = ee_bit_set(*byte, i - 1, bit);
        ee_bit_info_ms_inc(&(file->bit_info));
    }
}

static void
ee_file_copy_byte_bits_from_s(ee_file_t *file, ee_int_t start, ee_int_t end,
        ee_byte_t byte)
{
    for (ee_size_t i = start; i > end; --i) {
        ee_byte_t value = file->buffer[file->bit_info.current_byte];
        ee_size_t bit = ee_bit_get(byte, i - 1);
        file->buffer[file->bit_info.current_byte] = ee_bit_set(value,
                file->bit_info.current_bit, bit);
        ee_bit_info_ms_inc(&(file->bit_info));
    }
}
