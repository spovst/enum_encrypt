#ifndef SPLITTER_H
#define	SPLITTER_H

#include "common.h"
#include "block.h"

typedef struct ee_message_s {
    ee_char_t *chars;
    ee_size_t length;
} ee_message_t;

typedef struct ee_source_s {
    ee_char_t *prefix;
    ee_char_t *chars;
    ee_char_t *current_char;
    ee_size_t length;
    ee_size_t capacity;
} ee_source_t;

typedef struct ee_source_list_node_s {
    ee_source_t *source;
    struct ee_source_list_node_s *prev;
    struct ee_source_list_node_s *next;
} ee_source_list_node_t;

typedef struct ee_source_list_s {
    ee_size_t mu;
    ee_size_t size;
    ee_source_list_node_t *head;
    ee_source_list_node_t *tail;
} ee_source_list_t;

ee_int_t
ee_message_init(ee_message_t *message, ee_size_t length);
void
ee_message_deinit(ee_message_t *message);

ee_int_t
ee_source_init(ee_source_t *source, const ee_char_t *prefix, ee_size_t mu);
void
ee_source_deinit(ee_source_t *source);

ee_int_t
ee_source_append_char(ee_source_t *source, ee_char_t ch);

ee_int_t
ee_source_list_init(ee_source_list_t *list, ee_size_t mu);
void
ee_source_list_deinit(ee_source_list_t *list);

void
ee_source_list_clear(ee_source_list_t *list);
ee_int_t
ee_source_list_add(ee_source_list_t *list, ee_source_t *source);
ee_source_t *
ee_source_list_find(ee_source_list_t *list, const ee_char_t *window_start);
ee_size_t
ee_source_list_eval_message_length(ee_source_list_t *list);

ee_int_t
ee_source_split(ee_source_list_t *list, ee_message_t *message);
ee_int_t
ee_source_merge(ee_message_t *message, ee_source_list_t *list);

ee_int_t
ee_block_from_source(ee_block_t *block, ee_source_t *source,
        ee_size_t offset);
ee_int_t
ee_source_append_block(ee_source_t *source, ee_block_t *block);

#endif /* SPLITTER_H */
