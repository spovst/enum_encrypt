#include <stdlib.h>
#include <string.h>

#include "splitter.h"

#include "common.h"
#include "block.h"
#include "util.h"

static ee_int_t
ee_source_split_iter_s(ee_source_list_t *list, const ee_char_t *window_start);
static ee_int_t
ee_source_merge_iter_s(ee_char_t *ch, ee_source_list_t *list);

ee_int_t
ee_message_init(ee_message_t *message, ee_size_t length)
{
    message->length = length;
    message->chars = calloc(message->length, sizeof(*(message->chars)));
    if (NULL == message->chars) {
        return EE_ALLOC_FAILURE;
    }
    
    return EE_SUCCESS;
}

void
ee_message_deinit(ee_message_t *message)
{
    free(message->chars);
    ee_memset(message, 0, sizeof(*message));
}

ee_int_t
ee_source_init(ee_source_t *source, const ee_char_t *prefix, ee_size_t mu)
{
    source->capacity = 16;
    source->length = 0;
    source->chars = calloc(source->capacity, sizeof(*(source->chars)));
    if (NULL == source->chars) {
        return EE_ALLOC_FAILURE;
    }
    
    source->current_char = source->chars;
    
    source->prefix = calloc(mu, sizeof(*(source->prefix)));
    if (NULL == source->prefix) {
        free(source->chars);
        return EE_ALLOC_FAILURE;
    }
    
    if (NULL != prefix) {
        memcpy(source->prefix, prefix, mu);
    }
    
    return EE_SUCCESS;
}

void
ee_source_deinit(ee_source_t *source)
{
    free(source->prefix);
    free(source->chars);
    ee_memset(source, 0, sizeof(*source));
}

ee_int_t
ee_source_append_char(ee_source_t *source, ee_char_t ch)
{
    if (source->length == source->capacity) {
        ee_char_t *ptr = NULL;
        source->capacity *= 2;
        ptr = realloc(source->chars, source->capacity);
        if (NULL == ptr) {
            return EE_ALLOC_FAILURE;
        }
        
        source->chars = ptr;
        source->current_char = source->chars;
    }

    source->chars[source->length] = ch;
    source->length += 1;
    
    return EE_SUCCESS;
}

ee_int_t
ee_source_list_init(ee_source_list_t *list, ee_size_t mu)
{
    ee_source_list_node_t *node = NULL;
    
    node = calloc(1, sizeof(*node));
    if (NULL == node) {
        return EE_ALLOC_FAILURE;
    }
    
    list->mu = mu;
    list->size = 0;
    list->head = list->tail = node;
    
    return EE_SUCCESS;
}

void
ee_source_list_deinit(ee_source_list_t *list)
{
    ee_source_list_clear(list);
    free(list->tail);
    ee_memset(list, 0, sizeof(*list));
}

void
ee_source_list_clear(ee_source_list_t *list)
{
    ee_source_list_node_t *node = NULL;
    ee_source_list_node_t *temp = NULL;
    
    node = list->head;
    while (node != list->tail) {
        temp = node;
        node = node->next;
        ee_source_deinit(temp->source);
        free(temp->source);
        free(temp);
    }
    
    list->size = 0;
    list->head = list->tail;
}

ee_int_t
ee_source_list_add(ee_source_list_t *list, ee_source_t *source)
{
    ee_source_list_node_t *node = NULL;
    
    node = calloc(1, sizeof(*node));
    if (NULL == node) {
        return EE_ALLOC_FAILURE;
    }
    
    node->source = source;
    
    if (list->head == list->tail) {
        list->head = node;
    } else {
        node->prev = list->tail->prev;
        list->tail->prev->next = node;
    }
    
    node->next = list->tail;
    list->tail->prev = node;
    list->size += 1;
    
    return EE_SUCCESS;
}

ee_source_t *
ee_source_list_find(ee_source_list_t *list, const ee_char_t *window_start)
{
    ee_source_t *source = NULL;
    
    for (ee_source_list_node_t *n = list->head; n != list->tail; n = n->next) {
        if (0 == memcmp(n->source->prefix, window_start, list->mu)) {
            source = n->source;
            break;
        }
    }
    
    return source;
}

ee_size_t
ee_source_list_eval_message_length(ee_source_list_t *list)
{
    ee_size_t length = list->mu;
    
    for(ee_source_list_node_t *n = list->head; n != list->tail; n = n->next) {
        length += n->source->length;
    }
    
    return length;
}

ee_int_t
ee_source_split(ee_source_list_t *list, ee_message_t *message)
{
    ee_char_t *wstart = NULL;
    ee_char_t *mend = message->chars + message->length;
    
    wstart = message->chars;
    while (wstart + list->mu != mend) {
        ee_int_t status = ee_source_split_iter_s(list, wstart);
        if (EE_SUCCESS != status) {
            return status;
        }
        
        wstart += 1;
    }
    
    return EE_SUCCESS;
}

ee_int_t
ee_source_merge(ee_message_t *message, ee_source_list_t *list)
{
    ee_char_t *ch = message->chars;
    ee_char_t *mend = message->chars + message->length;
    
    memcpy(ch, list->head->source->prefix, list->mu);
    ch += list->mu;
    
    
    while (ch != mend) {
        ee_int_t status = ee_source_merge_iter_s(ch, list);
        if (EE_SUCCESS != status) {
            return status;
        }
        
        ch += 1;
    }
    
    return EE_SUCCESS;
}

ee_int_t
ee_block_from_source(ee_block_t *block, ee_source_t *source,
        ee_size_t offset)
{
    ee_size_t count = block->size;
    
    if (count + offset > source->length) {
        count = source->length - offset - 1;
    } else if (count + offset == source->length) {
        count -= 1;
    }
    
    ee_memset(block->chars, 0, block->size * sizeof(*(block->chars)));
    memcpy(block->chars, source->chars + offset, count);
    block->length = count;
    
    if (count < block->size) {
        return EE_FINAL_BLOCK;
    }
    
    return EE_SUCCESS;
}

ee_int_t
ee_source_append_block(ee_source_t *source, ee_block_t *block)
{
    ee_int_t status = EE_SUCCESS;
    
    for (ee_size_t i = 0; i < block->length; ++i) {
        status = ee_source_append_char(source, block->chars[i]);
        if (EE_SUCCESS != status) {
            break;
        }
    }
    
    return status;
}

static ee_int_t
ee_source_split_iter_s(ee_source_list_t *list, const ee_char_t *window_start)
{
    ee_int_t status;
    ee_source_t *source = ee_source_list_find(list, window_start);
    if (NULL == source) {
        source = calloc(1, sizeof(*source));
        if (NULL == source) {
            return EE_ALLOC_FAILURE;
        }

        status = ee_source_init(source, window_start, list->mu);
        if (EE_SUCCESS != status) {
            free(source);
            return status;
        }

        status = ee_source_list_add(list, source);
        if (EE_SUCCESS != status) {
            ee_source_deinit(source);
            free(source);
            return status;
        }
    }

    return ee_source_append_char(source, window_start[list->mu]);
}

static ee_int_t
ee_source_merge_iter_s(ee_char_t *ch, ee_source_list_t *list)
{
    ee_source_t *source = ee_source_list_find(list, ch - list->mu);
    if (NULL == source) {
        return EE_FAILURE;
    }

    *ch = *(source->current_char);
    source->current_char += 1;
    
    return EE_SUCCESS;
}
