#include <stdlib.h>
#include <string.h>

#include "source.h"

#include "common.h"
#include "block.h"
#include "util.h"

#define EE_CAPACITY_QUANT 256

static void
ee_source_list_clear_helper_s(ee_source_list_node_t *node);
static ee_source_list_node_t *
ee_source_list_insert_helper_s(ee_source_list_t *list,
        ee_source_list_node_t *root, ee_source_list_node_t *node);
static ee_source_t *
ee_source_list_find_helper_s(ee_source_list_node_t *node, ee_size_t mu,
        const ee_char_t *window_start);
static ee_bool_t
ee_source_list_eval_message_length_handler_s(ee_source_t *source, void *context);
static ee_bool_t
ee_source_list_traverse_helper_s(ee_source_list_node_t *node,
        ee_traverse_handler_t *handler, void *context);

static ee_byte_t
ee_source_list_node_height_s(ee_source_list_node_t *node);
static ee_int_t
ee_source_list_node_balance_factor_s(ee_source_list_node_t *node);
static void
ee_source_list_node_fix_height_s(ee_source_list_node_t *node);

static ee_source_list_node_t *
ee_source_list_node_rotate_left_s(ee_source_list_node_t *node);
static ee_source_list_node_t *
ee_source_list_node_rotate_right_s(ee_source_list_node_t *node);
static ee_source_list_node_t *
ee_source_list_balance_s(ee_source_list_node_t *root);

ee_int_t
ee_source_init(ee_source_t *source, const ee_char_t *prefix, ee_size_t mu)
{
    source->capacity = EE_CAPACITY_QUANT;
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
        source->capacity += EE_CAPACITY_QUANT;
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

void
ee_source_list_init(ee_source_list_t *list, ee_size_t mu)
{
    list->mu = mu;
    list->first = NULL;
    list->root = NULL;
}

void
ee_source_list_deinit(ee_source_list_t *list)
{
    ee_source_list_clear(list);
    ee_memset(list, 0, sizeof(*list));
}

void
ee_source_list_clear(ee_source_list_t *list)
{
    ee_source_deinit(list->first);
    free(list->first);
    ee_source_list_clear_helper_s(list->root);
}

ee_int_t
ee_source_list_insert(ee_source_list_t *list, ee_source_t *source)
{
    if (NULL == list->first) {
        list->first = source;
    } else {
        ee_source_list_node_t *node = NULL;

        node = calloc(1, sizeof(*node));
        if (NULL == node) {
            return EE_ALLOC_FAILURE;
        }

        node->source = source;
        node->height = 1;
        list->root = ee_source_list_insert_helper_s(list, list->root, node);
    }

    return EE_SUCCESS;
}

ee_source_t *
ee_source_list_find(ee_source_list_t *list, const ee_char_t *window_start)
{
    ee_source_t *result = NULL;

    if (NULL != list->first) {
        if (0 == memcmp(window_start, list->first->prefix, list->mu)) {
            result = list->first;
        } else {
            result = ee_source_list_find_helper_s(list->root, list->mu,
                    window_start);
        }
    }

    return result;
}

ee_size_t
ee_source_list_eval_message_length(ee_source_list_t *list)
{
    ee_size_t length = list->mu;

    ee_source_list_traverse(list, ee_source_list_eval_message_length_handler_s,
            &length);

    return length;
}

void
ee_source_list_traverse(ee_source_list_t *list, ee_traverse_handler_t *handler,
        void *context)
{
    if (EE_TRUE == handler(list->first, context)) {
        ee_source_list_traverse_helper_s(list->root, handler, context);
    }
}

static void
ee_source_list_clear_helper_s(ee_source_list_node_t *node)
{
    if (NULL != node) {
        ee_source_list_clear_helper_s(node->left);
        ee_source_list_clear_helper_s(node->right);
        ee_source_deinit(node->source);
        free(node->source);
        free(node);
    }
}

static ee_source_list_node_t *
ee_source_list_insert_helper_s(ee_source_list_t *list,
        ee_source_list_node_t *root, ee_source_list_node_t *node)
{
    ee_source_list_node_t *result = NULL;

    if (NULL == root) {
        result = node;
    } else {
        if (memcmp(node->source->prefix, root->source->prefix, list->mu) < 0) {
            root->left = ee_source_list_insert_helper_s(list, root->left, node);
        } else {
            root->right = ee_source_list_insert_helper_s(list, root->right, node);
        }

        result = ee_source_list_balance_s(root);
    }

    return result;
}

static ee_source_t *
ee_source_list_find_helper_s(ee_source_list_node_t *node, ee_size_t mu,
        const ee_char_t *window_start)
{
    ee_source_t *result = NULL;

    if (NULL != node) {
        ee_int_t cmp = memcmp(window_start, node->source->prefix, mu);

        if (cmp < 0) {
            result = ee_source_list_find_helper_s(node->left, mu, window_start);
        } else if (cmp > 0) {
            result = ee_source_list_find_helper_s(node->right, mu, window_start);
        } else {
            result = node->source;
        }
    }

    return result;
}

static ee_bool_t
ee_source_list_eval_message_length_handler_s(ee_source_t *source, void *context)
{
    *((ee_size_t *)context) += source->length;

    return EE_TRUE;
}

static ee_bool_t
ee_source_list_traverse_helper_s(ee_source_list_node_t *node,
        ee_traverse_handler_t *handler, void *context)
{
    ee_bool_t cont = EE_TRUE;

    if (NULL != node) {
        cont = ee_source_list_traverse_helper_s(node->left, handler, context);
        if (EE_FALSE == cont) {
            goto end;
        }

        cont = handler(node->source, context);
        if (EE_FALSE == cont) {
            goto end;
        }

        cont = ee_source_list_traverse_helper_s(node->right, handler, context);
    }

end:
    return cont;
}

static ee_byte_t
ee_source_list_node_height_s(ee_source_list_node_t *node)
{
    return (NULL == node) ? 0 : node->height;
}

static ee_int_t
ee_source_list_node_balance_factor_s(ee_source_list_node_t *node)
{
    return ee_source_list_node_height_s(node->right)
            - ee_source_list_node_height_s(node->left);
}

static void
ee_source_list_node_fix_height_s(ee_source_list_node_t *node)
{
    ee_byte_t lh = ee_source_list_node_height_s(node->left);
    ee_byte_t rh = ee_source_list_node_height_s(node->right);

    node->height = ((lh > rh) ? lh : rh) + 1;
}

static ee_source_list_node_t *
ee_source_list_node_rotate_left_s(ee_source_list_node_t *node)
{
    ee_source_list_node_t *result = NULL;

    result = node->right;
    node->right = result->left;
    result->left = node;
    ee_source_list_node_fix_height_s(node);
    ee_source_list_node_fix_height_s(result);

    return result;
}

static ee_source_list_node_t *
ee_source_list_node_rotate_right_s(ee_source_list_node_t *node)
{
    ee_source_list_node_t *result = NULL;

    result = node->left;
    node->left = result->right;
    result->right = node;
    ee_source_list_node_fix_height_s(node);
    ee_source_list_node_fix_height_s(result);

    return result;
}

static ee_source_list_node_t *
ee_source_list_balance_s(ee_source_list_node_t *node)
{
    ee_source_list_node_t *result = NULL;

    ee_source_list_node_fix_height_s(node);
    if (2 == ee_source_list_node_balance_factor_s(node)) {
        if (ee_source_list_node_balance_factor_s(node->right) < 0) {
            node->right = ee_source_list_node_rotate_right_s(node->right);
        }

        result = ee_source_list_node_rotate_left_s(node);
    } else if (-2 == ee_source_list_node_balance_factor_s(node)) {
        if (ee_source_list_node_balance_factor_s(node->left) > 0) {
            node->left = ee_source_list_node_rotate_left_s(node->left);
        }

        result = ee_source_list_node_rotate_right_s(node);
    } else {
        result = node;
    }

    return result;
}
