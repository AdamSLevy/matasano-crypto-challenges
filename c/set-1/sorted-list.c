#include <float.h>
#include <string.h>

#include <stdio.h>

#include "sorted-list.h"

int add_item(list_t *list, double score, void *data, size_t len)
{
    if (NULL == list || NULL == data || 0 == list->max_num_items) {
        return 1;
    }

    if (NULL == list->last) {
        if (NULL == (list->first = (list->last =
                        create_item(score, data, len, NULL, NULL)))) {
            return 1;
        }
        list->num_items++;
        return 0;
    } else {
        list_item_t *node = list->last;
        while (NULL != node &&
                (list->max ? (score > node->score) : (score < node->score))) {
            node = (list_item_t *)node->prev;
        }

        if (NULL == node) {
            list_item_t *old_first = list->first;
            if (NULL == (list->first =
                        create_item(score, data, len, NULL, old_first))) {
                return 1;
            }
            old_first->prev = list->first;
            list->num_items++;
        } else if (node != list->last || list->num_items < list->max_num_items) {
            list_item_t *new_node;
            if (NULL == (new_node =
                        create_item(score, data, len, node, (list_item_t *)node->next))) {
                return 1;
            }
            if (NULL != node->next) {
                list_item_t *old_next = (list_item_t *)node->next;
                old_next->prev = new_node;
            } else {
                list->last = new_node;
            }
            node->next = new_node;
            list->num_items++;
        }
    }

    while (list->num_items > list->max_num_items) {
        list_item_t *last = list->last;
        list->last = (list_item_t *)last->prev;
        if (NULL != list->last) {
            list->last->next = NULL;
        }
        if (NULL != list->free_data) {
            list->free_data(last->data);
        }
        free(last->data);
        free(last);
        list->num_items--;
    }

    return 0;
}

list_item_t *create_item(double score, void *data, size_t len,
        list_item_t *prev, list_item_t *next)
{
    if (NULL == data) {
        return NULL;
    }

    list_item_t *item = (list_item_t *)malloc(sizeof(list_item_t));
    item->data = malloc(len);
    if (NULL == item->data) {
        free(item);
        return NULL;
    }

    memcpy(item->data, data, len);
    item->score = score;
    item->prev = prev;
    item->next = next;

    return item;
}

int free_list(list_t *list)
{
    if (NULL == list) {
        return 1;
    }

    while (NULL != list->last) {
        list_item_t *last = list->last;
        list->last = (list_item_t *)last->prev;
        if (NULL != list->last) {
            list->last->next = NULL;
        }
        if (NULL != list->free_data) {
            list->free_data(last->data);
        }
        free(last->data);
        free(last);
        list->num_items--;
    }

    return 0;
}
