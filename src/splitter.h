#ifndef SPLITTER_H
#define	SPLITTER_H

#include "common.h"
#include "source.h"

typedef struct ee_message_s {
    ee_char_t *chars;
    ee_size_t length;
} ee_message_t;

ee_int_t
ee_message_init(ee_message_t *message, ee_size_t length);
void
ee_message_deinit(ee_message_t *message);

ee_int_t
ee_source_split(ee_source_list_t *list, ee_message_t *message);
ee_int_t
ee_source_merge(ee_message_t *message, ee_source_list_t *list);

#endif /* SPLITTER_H */
