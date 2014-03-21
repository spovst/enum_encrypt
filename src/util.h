#ifndef UTIL_H
#define	UTIL_H

#include "common.h"

void
ee_memset(void *mem, ee_int_t val, ee_size_t count);

ee_bool_t
ee_is_file_exists(const ee_char_t *name);

#endif	/* UTIL_H */

