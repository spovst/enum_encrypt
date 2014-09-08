#ifndef COMMON_H
#define	COMMON_H

#include <stddef.h>
#include <limits.h>

#define EE_BITS_IN_BYTE 8
#define EE_EVAL_BYTES_NUMBER(bits_number) \
        ((bits_number) / EE_BITS_IN_BYTE \
        + ((bits_number) % EE_BITS_IN_BYTE == 0 ? 0 : 1))

#define EE_ALPHABET_SIZE (CHAR_MAX - CHAR_MIN + 1)

#define EE_SUCCESS 0
#define EE_FAILURE 1
#define EE_ALLOC_FAILURE 2
#define EE_FILE_NOT_EXISTS 3
#define EE_FILE_OPEN_FAILURE 4
#define EE_FILE_READ_FAILURE 5
#define EE_FILE_WRITE_FAILURE 6
#define EE_END_OF_FILE 7
#define EE_INVALID_MODE 8
#define EE_INCORRECT_MODE 9
#define EE_FINAL_BLOCK 10

#define EE_TRUE ((ee_bool_t)1)
#define EE_FALSE ((ee_bool_t)0)

#define EE_MPZ_NULL { { 0, 0, NULL } }

typedef long int ee_int_t;
typedef size_t ee_size_t;
typedef char ee_char_t;
typedef char ee_bool_t;
typedef char ee_byte_t;

#endif /* COMMON_H */
