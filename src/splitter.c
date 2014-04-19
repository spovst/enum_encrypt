#include <stdlib.h>
#include <string.h>

#include "splitter.h"

#include "common.h"
#include "util.h"
#include "source.h"

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

    memcpy(ch, list->first->prefix, list->mu);
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

        status = ee_source_list_insert(list, source);
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
